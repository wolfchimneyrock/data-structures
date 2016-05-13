///////////////////////////////////////////////////////////////////////////////////
// xyzWidget.cpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #3
// Robert Wagner
// 2016-03-02
//
// to compile: g++ -std=c++11 xyzWidget.cpp -o xyzWidget
//     to run: ./xyzWidget < data.txt
//         or: ./xyzWidget data.txt
//
// note: I re-used a significant amount of the 'driver' code from assn #1 & #2 here
//
///////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <boost/format.hpp>
#include "LinearQueue.hpp"            // my linear linked list queue implementation

namespace xyzWidget {
///////////////////////////////////////////////////////////////////////////////////
// xyzWidget contains all classes for this project.
// Receipt class - will be stored in queue
// Store calss   - contains the main logic of the program
///////////////////////////////////////////////////////////////////////////////////

using std::endl;
using std::string;
using std::size_t;
using boost::format;
std::ostream& output = std::cout;

typedef LinearQueue<float> fVec;  // use a queue for transaction parameter list

// enumerate the types of data input records and their # of expected parameters
typedef enum tTypes { SALE, RECEIPT, PROMO } tTypes;
const std::map<const char, const tTypes> TRANSACTIONKEY =
    {{'S', SALE}, {'R', RECEIPT}, {'P', PROMO}};
const std::map<const tTypes, const size_t> PARAMETERCOUNT =
    {{SALE, 1}, {RECEIPT, 2}, {PROMO, 1}};

// format strings for output messages
format fmtSaleHeader   ("\n*** %5.0f Widgets order:\n");
format fmtSaleItem     ("*** %5.0f at %5.2f each  Sales: $%8.2f\n");
format fmtSaleFooter   ("***                Total Sales: $%8.2f\n");
format fmtPromoApplied ("*** %4.0f%% discount applied:    ($%8.2f)\n");
format fmtReceiptEcho  ("+++ %5.0f Widgets %s @ $%4.2f ea\n");
format fmtPromoEcho    ("!!! %4.0f%% Promotional discount next two orders !!!\n");
format fmtInsufficient ("!!! %5.0f Widgets unavailable\n");
format fmtRemaining    ("\nRemaining stock [%.0f price groups]:\n");
format fmtInvalidToken ("[%4i] Invalid token '%c', skipping.\n");
format fmtInputError   ("[%4i] Error: %s\n");

////////////////////////////////////////////////////////////////////////////////
// Receipt class
// this will be contained in the Queue
//
////////////////////////////////////////////////////////////////////////////////
class Receipt {
    private:
        float _qty;
        float _price;
    public:
        Receipt() {}
        ~Receipt() {}
        Receipt &operator=(const Receipt &other)
            { _qty = other._qty; _price = other._price; return *this; }

        Receipt(float qty_, float price_) : _qty(qty_), _price(price_)
            { Receipt(); }

        void removeQty(float qty) { _qty -= qty; if (qty < 0) qty = 0; }
        float getQty() const { return _qty; }
        float getPrice() const { return _price; }
}; // class Receipt

////////////////////////////////////////////////////////////////////////////////
// Store class
//
//
////////////////////////////////////////////////////////////////////////////////
class Store {
    private:
    int _promoRemaining;
    float _promoCoefficient;
    float _markup;
    LinearQueue<Receipt> _receiptList;
    size_t _saleCount, _promoCount, _receiptCount;
    static size_t _count;

    // the meat of the program
    int doSale (fVec& input) {
        float remaining = input.peek();
        float price, qty, amount, total = 0;
        output << fmtSaleHeader % remaining;
        while (remaining > 0 && !_receiptList.isEmpty()) {
            // get a reference to the front of the queue
            Receipt &current = _receiptList.peek();
            price = _markup * current.getPrice();
            qty = current.getQty();
            // entire sale can be handled by one receipt
            // no need to dequeue
            if (remaining < qty) {
                amount = price * remaining;
                current.removeQty(remaining);
                qty = remaining;
                remaining = 0;
            } else { // it will take all of this one and more receipts
               amount = price * qty;
               remaining -= qty;
               //delete current;
               _receiptList.deQueue();
            }
            total += amount;
            output << fmtSaleItem % qty % price % amount;
        }
        // apply a promo discount if present
        if (_promoRemaining > 0) {
            float discount = -1 * total * _promoCoefficient;
            total += discount;
            _promoRemaining--;
            output << fmtPromoApplied % (100 * _promoCoefficient) % discount;
        }
        if (total > 0) output << fmtSaleFooter % total;
        ++_saleCount;
        input.deQueue();
        if (remaining > 0) {  // partially unfulfilled
            output << fmtInsufficient % remaining << endl;
            return OUT_OF_STOCK;
        }
        output << endl;
        return SUCCESS;
    }

    int doReceipt (fVec& input) {
        float qty = input.peek();
        input.deQueue();
        float price = input.peek();
        Receipt R(qty, price);
        _receiptList.enQueue(R);
        output << fmtReceiptEcho % qty % "received" % price;
        ++_receiptCount;
        input.deQueue();
        return SUCCESS;
    }

    int doPromo (fVec& input) {
        _promoRemaining += 2;
        _promoCoefficient = input.peek() / 100;
        output << fmtPromoEcho % input.peek();
        ++_promoCount;
        input.deQueue();
        return SUCCESS;
    }

    public:
    static const int   SUCCESS        = 0;
    static const int   OUT_OF_STOCK   = -1;

    Store(float markup_) : _markup(markup_) {
        _count++;
        _saleCount = 0;
        _promoCount = 0;
        _receiptCount = 0;
        _promoRemaining = 0;
    }

    int handleInput(tTypes tType, const string& input) {
        char c;
        float temp;
        int result;
        fVec buff;
        std::stringstream SS(input); // convert to stringstream so we can tokenize

        SS >> c; // skip the code token
        while (SS >> temp) buff.enQueue(temp);    // read parameters
        auto pLength = PARAMETERCOUNT.find(tType);// check number of params
        if (buff.size() != pLength->second)
            throw std::runtime_error("Wrong # of parameters.");

        // do it
        switch (tType) {
            case SALE: {
                result = doSale(buff);
            } break;
            case RECEIPT: {
                result = doReceipt(buff);
            } break;
            case PROMO: {
                result = doPromo(buff);
            } break;
        }
        return result;
    }

    void finish() {
        output << fmtRemaining % _receiptList.size();
        while (!_receiptList.isEmpty()) {
            Receipt &current = _receiptList.peek();
            output << fmtReceiptEcho % current.getQty() % "remaining" % current.getPrice();
            _receiptList.deQueue();
        }
    }

}; // class Store
  // initialize static variables
    size_t Store::_count        = 0;

} // namespace xyzWidget

// main program
// significantly the same as assignment #2 main program

int main(int argc, char* argv[]) {
    using namespace xyzWidget;
    Store S(1.30);

    std::ifstream inFile;
    std::istream* inFileP = &std::cin;
    if (argc > 1) {
        inFile.open(argv[1]);
        if (inFile.good()) inFileP = &inFile;
        else inFile.close();
    }
    int result = Store::SUCCESS;
    char tCode = {0};
    size_t lineNo = 0;
    string line = "";

    while (std::getline(*inFileP, line)) {
        ++lineNo;
        if (line.size() == 0) continue;
        tCode = line.front();
        if (tCode == '#') continue;
        auto tType = TRANSACTIONKEY.find(tCode);
        if (tType == TRANSACTIONKEY.end()) {
            output << fmtInvalidToken % lineNo % tCode;
            continue;
        }
        try {
            //output << fmtInputEcho % line;
            result = S.handleInput(tType->second, line);
        }
        catch (std::exception &e) {
            output << fmtInputError % lineNo % e.what();
        }
        if (result != Store::SUCCESS) output << "!!!" << endl;
    }
    S.finish();
    if (inFile) inFile.close();
}
