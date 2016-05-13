/////////////////////////////////////////////////////////////////////////////////////
// abcHardware.cpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #1
// Robert Wagner
// 2016-02-06
// to compile: g++ -std=c++11 abcHardware.cpp -o abcHardware
// to run: ./abcHardware > invoices.txt
/////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/variant.hpp>
#include "strtk.hpp"                        // lightweight string token parser tool
                                            // https://github.com/ArashPartow/strtk
namespace abcHardware {

///////////////////////////////////////////////////////////////////////////////////////
// abcHardware contains all objects for this project.
///////////////////////////////////////////////////////////////////////////////////////

using std::endl;

///////////////////////////////////////////////////////////////////////////////////////
// format strings for invoice output
// to extend supported record types, a new format string must be added.
// see http://theboostcpplibraries.com/boost.format
///////////////////////////////////////////////////////////////////////////////////////

boost::format invoiceFmtHeader("%04i %=52s %10s  $%12.2f");
boost::format invoiceFmtOrder("%04i %-10s %-40s [%4d @ %8.2f]   $%12.2f");
boost::format invoiceFmtPayment("%04i %-10s %=60s $(%10.2f)");
boost::format invoiceFmtReturn("%04i %-10s %-40s [%4d @ %8.2f]   $(%10.2f) %2.0f%% FEE");
boost::format invoiceFmtFooter("%04i %70s  $%12.2f");

class Transaction  {

///////////////////////////////////////////////////////////////////////////////////////
// Transaction - Abstract Base class for derived classes Order, Payment, etc...
// I made this a pure abstract class so that derived classes do not exhibit 'slicing'
// effects, which allows a vector or map to contain a heterogeneous mix of objects of
// classes derived from this class without worrying about losing fields.  Thus we can
// read in a collection of orders and payments in random order and store them in one
// data structure.  I will use a multimap keyed to the customerId to
// facilitate:
//   1) associating transactions with customers for invoice processing.
//   2) checking for transactions with a non-existing customer.
//
// Note:  the 'adjustment' method calculates how this transaction effects the customer's
//        balance.  this handles the fact that payments are 'negative', so the new balance
//        calculation boils down to:
//
//           for [Transaction in Transactions]:
//              Customer += Transaction.adjustment()
//
//        without having to discriminate between payments, orders, etc...
//
///////////////////////////////////////////////////////////////////////////////////////

	protected:
	int _id;
	int _customerId;
	static int _count;		// total number of transactions processed in this batch
	friend std::ostream&
        operator<<(std::ostream& out, const Transaction& T);

	public:
	Transaction(int id_=0, int customerId_=0) :
        _id(id_), _customerId(customerId_)
        {++_count;}                  // keep track of total transactions
	virtual ~Transaction() {}        // virtual destructor

	// pure virtual functions must be overridden by derived classes
	virtual double adjustment() const = 0;
    virtual std::string invoice() const = 0;

	// Getters
	int getId() const { return _id; }
	int getCustomerId() const { return _customerId; }
	static int getCount() {return _count; }

	// Setters
	void setId(int id_) { _id = id_; }
	void setCustomerId(int customerId_) { _customerId = customerId_; }
}; // Class Transaction

// initialize static variables
int Transaction::_count = 0;

class Order: public Transaction {

////////////////////////////////////////////////////////////////////////////
// Order - derived from Transaction
// adding fields for quantity, cost, and name of item
////////////////////////////////////////////////////////////////////////////

	protected:
	int _qty;
	double _itemCost;
	std::string _itemName;

	public:
    Order(int id_=0, int customerId_=0, std::string itemName_="",
        int qty_=0, double itemCost_=0) :
        Transaction(id_, customerId_),				// call base constructor
        _itemName(itemName_), _qty(qty_), _itemCost(itemCost_)
        {}

	// Getters
	int getQty() const { return _qty; }
	double getItemCost() const { return _itemCost; }
	std::string const& getItemName() const { return _itemName; }

	// Setters
	void setQty(int qty_) { _qty = qty_; }
	void setItemCost(double itemCost_) { _itemCost = itemCost_; }
	void setItemName(std::string const& itemName_) { _itemName = itemName_; }

    // adjustment of order is qty * cost
	virtual double adjustment() const override { return _qty * _itemCost; }
	virtual std::string invoice() const override {
		return (invoiceFmtOrder % getId() % "ORDER" % getItemName() % getQty() %
                                getItemCost() % adjustment()).str();

	}
}; // Class Order

class Payment: public Transaction {

/////////////////////////////////////////////////////////////////////////////////////
// Payment - derived from Transaction
// adding field for payment amount
/////////////////////////////////////////////////////////////////////////////////////

	protected:
	double _amount;

	public:
    Payment(int id_=0,int customerId_=0, double amount_=0) :
            Transaction(id_, customerId_),  // call base constructor
            _amount(amount_)
            {}

	// Getters
	double getAmount() const { return _amount; }

	// Setters
	void setAmount(double amount_) { _amount = amount_; }

    // adjustment of a payment is negative
	virtual double adjustment() const override { return -1*_amount; }
	virtual std::string invoice() const override {
		return (invoiceFmtPayment % getId() % "PAYMENT" % std::string(40,'.') %
                adjustment()).str();
	}
}; // class Payment

class Return: public Order {

/////////////////////////////////////////////////////////////////////////////////////
// Return - derived from Order
// adding field for "restocking fee"
// and inverting "adjustment"
/////////////////////////////////////////////////////////////////////////////////////

    double _restockFee;
    public:
    Return(int id_=0, int customerId_=0, std::string itemName_="", int qty_=0,
            double itemCost_=0, double restockFee_=0) :
            Order(id_, customerId_,itemName_,qty_,itemCost_),
            _restockFee(restockFee_)
            {}

    // Getters
    double getRestockFee() const { return 100*_restockFee; }

    // Setters
    void setRestockFee(double restockFee_) { _restockFee = restockFee_; }

    virtual double adjustment() const override { return (_restockFee-1)*(_qty * _itemCost); }
    virtual std::string invoice() const override {
		return (invoiceFmtReturn % getId() % "RETURN" % getItemName() % getQty() %
                getItemCost() % adjustment() % getRestockFee()).str();

    }
}; // class Return


class Customer {

///////////////////////////////////////////////////////////////////////////////////////////
// Customer
// overrode the iostream << operator for invoice output
// overrode the += operator to commit transactions
///////////////////////////////////////////////////////////////////////////////////////////

	int _id;            // customer # i.e. _id assumed unique for map key
	std::string _name;  // customer name
	double _balance;    // current balance, without considering pending transactions
	static int _count;  // total number of customers in this session

    friend std::ostream& operator<<(std::ostream& out, const Customer& C);

	public:
    Customer(int id_=0, std::string name_="", double balance_=0) :
            _id(id_), _name(name_), _balance(balance_)
            { ++_count; }

	// Getters
	static int getCount() { return _count; }
	int getId() const { return _id; }
	std::string const& getName() const { return _name; }
	double getBalance() const { return _balance; }

	// Setters
	void setId(int id_) { _id = id_; }
	void setName(std::string const& name_) { _name = name_; }
	void setBalance(double balance_) { _balance = balance_; }

    std::string invoiceHeader() const
    { return (invoiceFmtHeader % getId() % getName() % "Previous Balance:" % getBalance()).str();}

    std::string invoiceFooter() const
    { return (invoiceFmtFooter % getId() % "New Balance:" % getBalance()).str();}

	Customer& operator+=(double amount)	  // manually adjust balance (positive)
		{ _balance += amount; }
	Customer& operator-=(double amount)	  // manually adjust balance (negative)
		{ _balance -= amount; }
    Customer& operator+=(Transaction & T) // adjust balance based on transaction
        { _balance += T.adjustment();}
}; // class Customer

// initialize static variables
int Customer::_count = 0;

////////////////////////////////////////////////////////////////////////////////////
// Overloaded IO operators to output invoices
// the customer overload outputs the invoice "header"
// the transaction overload prints out its invoice line only
////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<< (std::ostream& out, const abcHardware::Customer& C) {
	out << C.invoiceHeader() << endl;
	return out;
}

std::ostream& operator<< (std::ostream& out, const abcHardware::Transaction& T) {
    out << T.invoice() << endl;
    return out;
}

///////////////////////////////////////////////////////////////////////////////////
// collection definitions
// to extend functionality by adding new transaction types, you must do 4 things:
//   1) implement your new class with a base class of 'Transaction' or a descendent
//   2) the new class must override the inherited adjustment() and invoice() methods
//   3) add the new class to the 'Multi' template below
//   4) add a new case in the readTransaction() switch statement to parse the data
//   5) add a new boost::format string for invoice output
//
// I have extended the basic assignment specification with a 'Return' transaction
// which inherits from 'Order' and adds a 'restocking fee'
////////////////////////////////////////////////////////////////////////////////////

using Multi = boost::variant<Order,Payment,Return>;

typedef std::map<int, Customer> customerMap;
typedef std::multimap<int, Multi> transactionMap;


int readTransactions(const std::string &fileName, transactionMap& transactions ) {

/////////////////////////////////////////////////////////////////////////////////
// readTransactions
// parameters:
// string fileName of file containing transactions
// reference to unordered_multimap to put parsed transactions into
//
// return value: int # of lines of the file parsed, or -1 for file read error
/////////////////////////////////////////////////////////////////////////////////

	std::ifstream inFile (fileName);
	if(!inFile) {
		std::cerr
		<< "Unable to open transactions file: " << fileName
		<< endl;
		return -1;
	}
	int count=0;
	std::string line;
	while(std::getline(inFile, line)) {
        if(line.size()==0) continue;        // skip blank lines without counting
        if(line.front()=='#') continue;     // skip 'comments' without counting
        try {
		char type_=0; int id_=0; int customerId_=0; double amount_=0; int qty_=0;
             std::string name_=""; double fee_=0;
		++count;
		switch(line.front()) {
		    case 'O':
                strtk::parse(line,"\t",type_,customerId_,id_,name_,qty_,amount_);
		        if(id_ != 0) transactions.emplace(customerId_,
                                Order(id_,customerId_,name_,qty_,amount_));
                break;
		    case 'P':
                strtk::parse(line,"\t",type_,customerId_,id_,amount_);
		        if(id_ != 0) transactions.emplace(customerId_,
                                Payment(id_,customerId_,amount_));
                break;
            case 'R':
                strtk::parse(line,"\t",type_,customerId_,id_,name_,qty_,amount_,fee_);
                if(id_ != 0) transactions.emplace(customerId_,
                                Return(id_,customerId_,name_,qty_,amount_,fee_));
                break;
		    default:
		        std::cerr
		        <<"Unknown entry: " << fileName
		        << " line " << count
		        << endl;
            }
		}
		catch(std::invalid_argument& e) {
            std::cerr
            << e.what()
            << " in file " << fileName
            << " line " << count
            << endl;
            throw;
        }
	}
	return count;
} // readTransactions

int readCustomers(const std::string &fileName, customerMap &customers) {

/////////////////////////////////////////////////////////////////////////////////
// readCustomers
// parameters:
// string fileName of file containing customers
// reference to map to put parsed customers into
//
// return value: int # of lines of the file parsed, or -1 for file read error
/////////////////////////////////////////////////////////////////////////////////

	std::ifstream inFile (fileName);
	if(!inFile) {
		std::cerr
		<< "Unable to open customers file: " << fileName
		<< endl;
		return -1;
	}
	int count = 0;
	std::string line;
	while(std::getline(inFile, line)) {
        if(line.size()==0) continue;    // skip blank lines without counting
        if(line.front()=='#') continue; // skip 'comments' without counting
        try {
            int id_=0; std::string name_=""; double balance_=0;
            ++count;
			strtk::parse(line,"\t",id_,name_,balance_);
			if(id_ != 0) customers.emplace(id_,Customer(id_,name_,balance_));
		}
		catch(std::invalid_argument& e) {
            std::cerr
            << e.what() << " in file " << fileName
            << " line " << count
            << endl;
        }
	}
	return count;
} // readCustomers

//////////////////////////////////////////////////////////////////////////////
// Helper functions etc...
//////////////////////////////////////////////////////////////////////////////

struct genericAdjustment : boost::static_visitor<double> {

/////////////////////////////////////////////////////////////////////////////////////////
// this is boost library's recommended idiom for calling a known method i.e. adjustment()
// from an indeterminate class i.e. Order, Payment, etc...
/////////////////////////////////////////////////////////////////////////////////////////

    template <class T> double operator()(const T & operand) const {
        return operand.adjustment();
    }
};

template<typename fwdIt> std::set<int> uniqueKeys(fwdIt begin, fwdIt end) {

///////////////////////////////////////////////////////////////////////////////////
// return a set of the keys's that are present in the collection
// apparently the proper idiom is to pass begin / end iterators and loop through
// who would have guessed
///////////////////////////////////////////////////////////////////////////////////

    std::set<int> uniques;
    while(begin != end) {
            uniques.insert(begin->first);
            ++begin;
    }
    return uniques;
}

// draw a full width bar
std::string drawBar(char c) { return std::string(90,c)+'\n';}

} // namespace abcHardware

int main(int argc, char* argv[]) {

///////////////////////////////////////////////////////////////////////////////////
// Main program
// Basic Steps:
//   1. read customer file
//   2. read transactions file
//   3. determine which customers have transactions
//      A. get unique set of customers from customer file
//      B. get unique set of customers from transaction file
//      C. intersect set for valid transaction
//      D. difference set to report dangling transactions
//   4. loop through customers:
//         A. output invoice header with previous balance
//         B. loop through transactions:
//           i.  output transaction to invoice
//           ii. commit transaction to customer record
//         C. finalize invoices with new balance due
//   5. update customer file
///////////////////////////////////////////////////////////////////////////////////

	using namespace abcHardware;

	customerMap customers;
    transactionMap transactions;

	int linesParsed, numInserted;

    // I could use command line parameters but I'm too lazy

	std::string customerFile = "customers.txt";
	std::string transactionFile = "transactions.txt";

	// 1. read the customer file and make sure everything was read fine

	linesParsed = readCustomers(customerFile, customers);
	numInserted = Customer::getCount();
	if (linesParsed != numInserted) {
        std::cerr
        << "Irregularity reading Customer file.  There may be missing data.  "
        << "Continuing anyway."
        << endl;
	}

    std::clog
    << "Read " << linesParsed << " lines "
    << "and parsed " << numInserted << " Customers."
    << endl;

	// 2. read the transaction file and make sure everything was read fine

	linesParsed = readTransactions(transactionFile, transactions);
	numInserted = Transaction::getCount();
	if (linesParsed != numInserted) {
        std::cerr
        << "Irregularity reading Transaction file.  There may be missing data.  "
        << "Continuing anyway."
        << endl;
	}

    std::clog
    << "Read " << linesParsed << " lines "
    << "and parsed " << numInserted << " Transactions."
    << endl;

	// if there is nothing to do, exit immediately

	if(0==Transaction::getCount() || 0==Customer::getCount()) {
        std::cerr
        << "Nothing to do."
        << endl;
        return 0;
    }

    // 3. Determine which customers have transactions.
    // 3A. get the unique set of customerId's that are in the transactions

    std::set<int> cInT = uniqueKeys(transactions.begin(),transactions.end());

    // 3B. get a unique set of customerId's that are in the customers

    std::set<int> cInC = uniqueKeys(customers.begin(), customers.end());

    // 3C. intersection (valid customers)
    //     this would be so easy in python

    std::set<int> cInBoth;
    std::set_intersection(cInT.begin(), cInT.end(),cInC.begin(),cInC.end(),
        std::inserter(cInBoth,cInBoth.begin()));

    // 3D. difference cInT - cInC == invalid customers in transactions list

    std::set<int> cInTnotC;
    std::set_difference(cInT.begin(), cInT.end(), cInC.begin(), cInC.end(),
        std::inserter(cInTnotC,cInTnotC.begin()));

    if(!cInTnotC.empty())
        std::cerr
        << "Transaction(s) present with invalid Customer #.  Some data may be lost.  "
        << "Continuing anyway."
        << endl;

    // 4. loop through the transactions for invoice output.
    //      outer loop: customers with transactions
    //      inner loop: transactions for that customer

    std::pair<transactionMap::iterator,transactionMap::iterator> range;
    for (auto C: cInBoth) {

        // 4A. print preliminary invoice customer "header"
        std::cout << customers[C];
        std::cout << drawBar('=');

        // filter transactions to just those for the current customer in inner loop
        range = transactions.equal_range(C);

        // 4B. loop through current customer's transactions
        for (auto T=range.first; T != range.second; ++T) {

            // 4B(i). print transaction to invoice
            std::cout << T->second;

            // 4B(ii). commit transaction to customer
            // this would also be way easier in python
		customers[C] += T->second.get();
//            customers[C] += boost::apply_visitor(genericAdjustment(),
//                                                 T->second);
        }

        // 4C. output final balance to invoice
        std::cout
        << drawBar('=')
        << customers[C].invoiceFooter()
        << endl << endl << endl;
    }
    return EXIT_SUCCESS;
    // all memory deallocation is done automatically. Thanks C++11 emplace!!
}

