/////////////////////////////////////////////////////////////////////////////////////
// SupplyChain.cpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #2
// Robert Wagner
// 2016-02-21
// to compile: g++ -std=c++11 -O3 SupplyChain.cpp -o SupplyChain
//     to run: ./SupplyChain < data.txt
//         or: ./SupplyChain data.txt
/////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <Eigen/Dense>          // Eigen Dense vector class has builtin arithmatic ops
                                // see http://eigen.tuxfamily.org for documentation
#include <boost/format.hpp>     // sprintf-like functionality for iostream
#include <boost/shared_ptr.hpp> // to wrap references where persistence is required

namespace SupplyChain {

//////////////////////////////////////////////////////////////////////////////////////
// SupplyChain contains all classes and objects for this project.
// even though the specifications only mention 3 items and 5 warehouses, I am writing
// this assuming in a real life situation you could have thousands of items and
// a large number of stock lockations.  I will try to minimize data copying as much as
// possible, and rely as much as possible on Eigen/Dense matrix operations which can
// be optimized to execute on whatever vector processing unit is available (ie. SSE)
//////////////////////////////////////////////////////////////////////////////////////

using std::endl;
using std::string;
using std::size_t;
std::ostream& output = std::cout;

////////////////////////////////////////////////////////////////////////////////////////
// note: storing the qty as a float so that qty*cost dot product requires no casts,
// and thus can be optimized to the SSE or AVX vector unit.  also I will utilize a float
// valued qty to perform the markup for the warehouse transfers i.e. markup * qty so
// we don't have to mess with the constant cost vector
// have to use -O3 compiler flag to use SSE/AVX
////////////////////////////////////////////////////////////////////////////////////////

class Warehouse;
class supplyChain;

typedef Eigen::Array<float, 1, -1>  qtyVector;   // float valued "array" - elementwise operations
typedef Eigen::Matrix<float, 1, -1> costVector;  // float valued vector  - matrix operations only
using RefWH = boost::shared_ptr<Warehouse>;      // wrap the warehouse for persistence


typedef enum tTypes { ORDER, PRICES, SHIPMENT } tTypes;

// this will be used by the input parser to categorize input data
const std::map<const char, const tTypes> TRANSACTIONKEY =
    {{'O', ORDER}, {'P', PRICES}, {'S', SHIPMENT}};

// format strings for output messages
boost::format fmtInputEcho      ("\n[%4i] %s\n");
boost::format fmtTransaction    ("  Price of Order: $%.2f\n");
boost::format fmtTransfer       ("  %-4.0f of item %4i shipped from %-15s to %-15s\n");
boost::format fmtNoSuchWarehouse("  Warehouse %s not found.\n");
boost::format fmtUnfilledOrder  ("  !!! Order unfilled for item %i !!!\n");
boost::format fmtInvalidToken   ("[%4i] Invalid token '%c', skipping.\n");
boost::format fmtInputError     ("[%4i] Input error: %s\n");
boost::format fmtWarehousePrint ("  ***%-15s new qty: %s ***\n");
boost::format fmtStatistics     ("\n%20s %6i\n%20s %6i\n%20s %6i [%5.2f%%]\n%20s %6i [%5.2f%%]\n");

class Warehouse {
///////////////////////////////////////////////////////////////////////////////////
//  Warehouse
//  A single warehouse.  most of the interesting stuff happens in SupplyChain.
///////////////////////////////////////////////////////////////////////////////////
    private:
        static size_t  _count;
        string        _name;
        qtyVector     _stock;
    public:
      // copy the passed name string and initialize the _stock vector with passed size
        Warehouse(const string& name_, size_t itemCount_) :
            _name(name_)
            { ++_count; qtyVector _stock(itemCount_); }

        // copy the passed name string and copy the passed qtyVector
        Warehouse(const string& name_, const qtyVector& stock_) :
            _name(name_), _stock(stock_)
            { ++_count;}

        ~Warehouse() { }

      // Getters
         float getItemQty(size_t i) const {
             if (i < _stock.size())
                 return _stock[i];
             else return 0;
         }

         const string& getName() const { return _name; }

         bool canFulfill(const qtyVector& orderQty) {
           // qtyVector leftoverQty   = _stock - orderQty;
            // Eigen vector supports R-cran like broadcast and reduce paradigm ...
            // useful in this case, see http://eigen.tuxfamily.org/dox-devel/
            // if any of the items' stock is insufficient for the order,
            // this returns false.
            return ((_stock >= orderQty ).all());
         }

         qtyVector orderDeficit(const qtyVector& orderQty) {
           /////////////////////////////////////////////////////////////////////////////////////////
           // calculate how much we need to fulfill the order.
           //  1. take the difference between existing stock and order
           //  2. replace values > 0 with zero
           //  3. take absolute value
           /////////////////////////////////////////////////////////////////////////////////////////
            qtyVector zeroes = qtyVector::Zero(_stock.size());
            qtyVector leftover = _stock - orderQty;
            qtyVector deficit = (leftover.min(zeroes)).abs();
            return deficit;
         }
      // Setters
        void addStock(const qtyVector& supply) {
            _stock += supply;
        }

        void removeStock(const qtyVector& order) {
          // if this would result in a negative stock for any item throw exception
            if (!canFulfill(order)) throw std::invalid_argument("");
            _stock -= order;
        }

      // allow iostream output either wrapped or unwrapped
        friend std::ostream& operator<<(std::ostream& out, const RefWH W);
        friend std::ostream& operator<<(std::ostream& out, const Warehouse& W);
    };  // class Warehouse

    // Initialize static variables
    size_t Warehouse::_count = 0;

    std::ostream& operator<<(std::ostream& out,const RefWH W) {
        return out << fmtWarehousePrint % W->_name % W->_stock;
    }
    std::ostream& operator<<(std::ostream& out, const Warehouse& W) {
        return out << fmtWarehousePrint % W._name % W._stock;
    }
///////////////////////////////////////////////////////////////////////////////////
//  Structure definitions
//  warehouseMap stores the warehouse references, keyed by name
//  qtyTuple and qtySortMap are used to sort the warehouses by item qty
//  qtyXferMap is used to determine whom to transfer from
///////////////////////////////////////////////////////////////////////////////////

    typedef std::map<string, RefWH>     warehouseMap;
    typedef std::pair<RefWH, float>     qtyTuple;
    typedef std::vector<qtyTuple>       qtySortMap;
    typedef std::map<RefWH, qtyVector>  qtyXferMap;

    struct byQtyDescending {  // functor to sort qtyTuple descending
        bool operator() (const qtyTuple &left, const qtyTuple &right) {
            return left.second > right.second;
        }
    };

// dereference object whether wrapped in smart pointers or not
// used to access Warehouse member functions through the warehouseMap

template <typename T>
T& deref(T& x) { return x; }

template <typename T>
T& deref(T* x) { return *x; }

class supplyChain {
///////////////////////////////////////////////////////////////////////////////////
//  SupplyChain
//  This class will contain a data structure of the warehouses and be responsible
//  for all inter-warehouse logistics, so that the client program can be agnostic
//  about this.  A client should be able to request an order from the SupplyChain
//  without having to know how the items are shipped between warehouses.
//
//  warehouses are stored in a map container keyed to the warehouse name.
//  item prices are stored in a Eigen vector that should maintain the same order
//  as the vector of quantities in the Transaction objects.  this will faciliate
//  calculation of the 'price' of an order by vector dot product between the price
//  vector and an order's quantity vector, multiplied by markup scalar as needed.
//
///////////////////////////////////////////////////////////////////////////////////
    private:
        size_t          _itemCount;     // number of different item types
        warehouseMap    _warehouses;    // map of warehouses
        costVector      _prices;        // vector of prices for items
        float           _markupC;       // markup scalar for warehouse transfers
        static size_t   _orderCount;    // total order count
        static size_t   _cancelCount;   // total cancelled count
        static size_t   _transferCount; // total order transfers
        static size_t   _shipmentCount; // total shipments
    public:
        supplyChain(const float markupC_ = 0.1) :
            _markupC(markupC_)
        {
          // initialize contents
            warehouseMap _warehouses();
            costVector _prices();
            _itemCount = 0;
        }

        virtual ~supplyChain() { }

      // Getters
        size_t getItemCount() const { return _itemCount; }
        const costVector getPrices() const { return _prices; }
        size_t getPriceCount() const { return _prices.size(); }
        size_t getWarehouseCount() const { return _warehouses.size(); }
        float getMarkup() const { return _markupC; }
       // Setters
        void setItemCount(const size_t count) { _itemCount = count; }
        void setPrices(const costVector& prices_)
            { _prices = prices_; }
        void setMarkup(const float markupC_)
            { _markupC = markupC_; }

        // find the warehouse with the most of item #i
        // returns a reference to the matched warehouse or NULL
        // if it matches the warehouse requesting, skip to next highest
        const RefWH findMaxItem(const RefWH from, size_t i, float qtyNeeded) {
            if (i >= _prices.innerSize()) throw std::out_of_range("");
            // if there is none or only one warehouse ... we can't transfer to ourself.
            if (_warehouses.size() < 2) throw std::range_error("");

            // fill a vector of tuples (qty) with warehouse reference & item i qty.
            qtySortMap qty;
            for (const auto &W: _warehouses) {
                qty.push_back(std::make_pair(W.second, deref(W.second)->getItemQty(i)));
            }

            // sort the warehouses by qty of item i.
            // outcome: index 0 should be highest.:
            // if index 0 is who is requesting transfer, return 1.
            RefWH result;
            std::sort(qty.begin(), qty.end(), byQtyDescending());
            if (qty[0].first == from) result = qty[1].first;
            else result = qty[0].first;
            if (result->getItemQty(i) < qtyNeeded) throw std::range_error("");
            return result;
        }

      // execute a transfer of items between warehouses.  returns true if
      // successful, or false if unsuccessful.
      // stock levels should be checked before doing this.
        bool transfer(RefWH from, RefWH to, const qtyVector& qty) {
            try {
                // the stock removal could fail, so try it first.
                from->removeStock(qty);
                to->addStock(qty);
            } catch(std::exception& e) { return false; };
            for (size_t i=0; i < qty.size(); i++)
                if (qty[i] > 0.0) {
                    output << fmtTransfer % qty[i] % (i + 1) % from->getName() % to->getName();
                    ++_transferCount;
                }
            return true;
        }

        // process an order.  returns value if successful or -1 if unsuccessful.
        // the meat of the project.
        float order(const string& name, qtyVector& orderQty) {

            // find() either returns the matched object in the map or end() if not found
            warehouseMap::iterator it = _warehouses.find(name);
            warehouseMap::iterator noWH = _warehouses.end();  // this is equivalent to 'not found'

            _orderCount += orderQty.nonZeros();

            // if not found in map print error and exit order()
            if (it == noWH) {
                output
                << fmtNoSuchWarehouse % name
                << fmtUnfilledOrder % -1;
                return 0.0;
            }

            RefWH WH = it->second;
            RefWH fromWH;

            float     price         = 0;

            if (!WH->canFulfill(orderQty)) {
              // there was a deficit somewhere.  iterate through the deficits and either:
              // 1) add item and source to transfer map
              // 2) cancel item if not enough available in another warehouse
                qtyVector deficitQty = WH->orderDeficit(orderQty);
                qtyXferMap xfer;
                qtyVector zeroes = qtyVector::Zero(getItemCount());
                for (size_t i = 0; i < deficitQty.size(); i++) {     // iterate through items
                    if (deficitQty[i] > 0) {                         // item i needs transfer(s)
                        qtyVector tempVec = zeroes;
                      // find which warehouse to transfer from
                        try { fromWH = findMaxItem(WH, i, deficitQty[i]); }
                      // if not enough qty available or no other warehouses, cancel order, skip.
                        catch(std::exception& e) {
                            ++_cancelCount;
                            orderQty[i] = 0;
                            deficitQty[i] = 0;
                            output << fmtUnfilledOrder % (i + 1);
                            continue;
                        }

                        tempVec[i] += deficitQty[i];
                      // if not there, add this item's warehouse to the xferMap
                      // and initialize other qty's to zero
                        if (xfer.find(fromWH) == xfer.end()) {
                            xfer[fromWH] = tempVec;
                        } else // or if there, just increase the value of this item
                            xfer[fromWH] += tempVec;
                  }

                }
                // perform transfers stored in xfer
                for (const auto& fromWH: xfer) {
                    transfer(fromWH.first, WH, fromWH.second);
                    output << fromWH.first;
                }
                // now cancelled item orders should have a 0 entry in orderQty and deficitQty.
                // perform order.
                WH->removeStock(orderQty);

                output << WH;  // print the outcome
                // Eigen .matrix() casting has no run-time penalty but is required to perform
                // linear alg. on an Eigen Array instead of elementwise operations.
                // the price vector we initialized for matrix operations since thats all it does
                qtyVector markedUp = orderQty + getMarkup() * deficitQty;
                price = (markedUp.matrix()).dot(getPrices()); // calc price, apply transfer markup

            } else {  // no deficits, no transfers.  perform order.

                WH->removeStock(orderQty);
                output << WH;
                price = (orderQty.matrix()).dot(getPrices());
            }
            // whatever happened, there should be a price calculated by this point.
            return price;
        }

        // process a shipment.
        void ship(const string& name, const qtyVector& qty) {
            warehouseMap::iterator it = _warehouses.find(name);
            // if the warehouse exists, then add this shipment to its stock
            if (it != _warehouses.end()) it->second->addStock(qty);
            else {
               // if the warehouse doesn't exist yet, create it
               // use a shared pointer so its not limited to this method scope
               // as we won't be performing any copying from here on out
               RefWH newWH(new Warehouse(name, qty));
               _warehouses[name] = newWH;
            }
            output << _warehouses.find(name)->second;  // print the outcome
            _shipmentCount += qty.nonZeros();
        }

        void handleInput(tTypes tType, const string& input) {
            char c;
            float temp;
            string name;
            std::vector<float> buff;
            std::stringstream SS(input);

            SS >> c; // skip the code token

            // get the warehouse name if appropriate
            if (tType == ORDER || tType == SHIPMENT) {
                std::getline(SS, name, '\t');
            }
            // read the numbers into a buffer, can be an arbitrary number.
            while (SS >> temp) buff.push_back(temp);
            size_t size = buff.size();
            if (size < 1) throw std::invalid_argument("");
            if (size < getItemCount()) throw std::invalid_argument("");

            switch(tType) {
                case PRICES: {
                    costVector p = costVector::Map(buff.data(),1,size);
                    setItemCount(size);
                    setPrices(p);
                }
                break;
                case ORDER: {
                    qtyVector oq = costVector::Map(buff.data(),1,size);
                    temp = order(name, oq);
                    output << fmtTransaction % temp;
                }
                break;
                case SHIPMENT: {
                    qtyVector sq = qtyVector::Map(buff.data(),1,size);
                    ship(name, sq);
                }
                break;
            }

        }
    void printStatistics() {
        if (_orderCount > 0) {
            float transferRatio = (float)(100 * _transferCount / _orderCount);
            float cancelRatio = (float)(100 * _cancelCount / _orderCount);
            output << fmtStatistics
                % "Total Shipments" % _shipmentCount
                % "Total Orders" % _orderCount
                % "Total Transfers" % _transferCount % transferRatio
                % "Total Unfilled" % _cancelCount % cancelRatio;
        }
    }
};  // class SupplyChain
  // initialize static variables
    size_t supplyChain::_orderCount    = 0;
    size_t supplyChain::_cancelCount   = 0;
    size_t supplyChain::_transferCount = 0;
    size_t supplyChain::_shipmentCount = 0;

}  // namespace SupplyChain

int main(int argc, char* argv[]) {
    using namespace SupplyChain;
    supplyChain SC;

    // lines are processed as they are encountered, not in batches,
    // so file read by line is main loop.
    // if no file name passed as parameter, or invalid filename,
    // read from standard input.
    std::ifstream inFile;
    std::istream* inFileP = &std::cin;
    if (argc > 1) {
        inFile.open(argv[1]);
        if (inFile.good()) inFileP = &inFile;
        else inFile.close();
    }
    char tCode = {0};
    size_t lineNo = 0;
    string line = "";

    while(std::getline(*inFileP, line)) {
        ++lineNo;
        if (line.size()==0) continue;                   // skip blank lines
        tCode = line.front();
        if (tCode == '#') continue;                     // skip comments
        auto tType = TRANSACTIONKEY.find(tCode);
        if (tType == TRANSACTIONKEY.end()) {            // skip unknown tokens
            output << fmtInvalidToken % lineNo % tCode; // but complain
            continue;
        }
        try {
            output << fmtInputEcho % lineNo % line;     // echo the transaction
            SC.handleInput(tType->second, line);        // actually do the work
        } catch(std::exception &e) {
            output << fmtInputError % lineNo % e.what();// or kvetch about it
        }
    }
    SC.printStatistics();
    if (inFile) inFile.close();
    return EXIT_SUCCESS;
}
