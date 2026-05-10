#ifndef PATIENT_H
#define PATIENT_H

#include "Person.h"
#include <fstream>
using namespace std;

class Patient : virtual public Person {
private:
    int    age;
    char   gender;
    int*   contact;
    double balance;
    int    bills;     // count of currently Unpaid bills

public:

    Patient() : Person(), age(0), gender('\0'), balance(0.0), bills(0) {
        contact    = new int[11];
        for (int i = 0; i < 11; i++) contact[i] = 0;
    }

    Patient(int _id, char* _name, char* _password,
            int _age, char _gender, int* _contact, double _balance)
        : Person(_id, _name, _password),
          age(_age), gender(_gender), balance(_balance), bills(0) {
        contact = new int[11];
        for (int i = 0; i < 11; i++) contact[i] = _contact[i];
    }

    Patient(const Patient& obj) : Person(obj), age(obj.age), 
    gender(obj.gender), balance(obj.balance), bills(obj.bills) {
        contact = new int[11];
        for (int i = 0; i < 11; i++) contact[i] = obj.contact[i];
    }

    Patient& operator=(const Patient& obj) {
        if (this == &obj) return *this;

        Person::operator=(obj);

        age = obj.age;
        gender = obj.gender;
        balance = obj.balance;
        bills = obj.bills;

        delete[] contact;

        contact = new int[11];
        for (int i = 0; i < 11; i++) contact[i] = obj.contact[i];

        return *this;
    }

    ~Patient() {
        delete[] contact;
    }

    friend ostream& operator<<(ostream& out, const Patient& obj) {
        out << "ID: " << obj.id << " | Name: " << obj.name;
        out << " | Age: " << obj.age;
        out << " | Gender: ";

        if (obj.gender == 'M') out << "Male";
        else out << "Female";

        out << " | Contact: ";

        for (int i = 0; i < 11; i++) out << obj.contact[i];
        
        out << " | Balance: PKR " << obj.balance;
        out << " | Unpaid Bills: " << obj.bills;
        
        return out;
    }

    Patient& operator+=(double amount) {
        balance += amount;
        return *this;
    }

    Patient& operator-=(double amount) {
        if (balance >= amount) {
            balance -= amount;
        } 
        else {
            cout << "Insufficient balance - setting to 0." << endl;
            balance = 0.0;
        }
        return *this;
    }

    bool operator>=(double fee) const {
        return balance >= fee;
    }

    void writeToFile(ofstream& file) const {
        file << id << "," << name << "," << age << "," << gender << ",";

        for (int i = 0; i < 11; i++) file << contact[i];

        file << "," << password << "," << balance;
    }

    // Getters
    int getID() const { 
        return id; 
    }
    char*  getName() const { 
        return name; 
    }
    int getAge() const { 
        return age; 
    }
    char getGender() const { 
        return gender; 
    }
    int* getContact() const { 
        return contact; 
    }
    double getBalance() const { 
        return balance; 
    }
    int getBills() const { 
        return bills; 
    }

    void setBalance(double newBalance)  { 
        balance = newBalance; 
    }

    // Call when an unpaid bill is created / paid / cancelled
    void incrementBills() { 
        bills++; 
    }
    void decrementBills() { 
        if (bills > 0) bills--; 
    }
    void setBills(int count) { 
        bills = count; 
    }
};

#endif