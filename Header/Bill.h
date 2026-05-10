#ifndef BILL_H
#define BILL_H

#include "constants.h"

class Bill {
    private:
        int    id;
        int    patientID;
        int    appointmentID;
        double amount;
        char*  status;    // "unpaid" | "paid" | "cancelled"
        char*  date;      // DD-MM-YYYY

        static char* dupStr(const char* src) {
            int len = lenArr(src);
            char* dst = new char[len + 1];
            for (int i = 0; i <= len; i++) dst[i] = src[i];
            return dst;
        }

        static bool strEq(const char* a, const char* b) {
            int i = 0;
            while (a[i] != '\0' && b[i] != '\0') {
                if (a[i] != b[i]) return false;
                i++;
            }
            return (a[i] == '\0' && b[i] == '\0');
        }

    public:

        Bill() : id(0), patientID(0), appointmentID(0), amount(0.0) {
            status = new char[1]; status[0] = '\0';
            date   = new char[1]; date[0]   = '\0';
        }

        Bill(int _id, int _patientID, int _appointmentID,
            double _amount, const char* _status, const char* _date)
            : id(_id), patientID(_patientID),
            appointmentID(_appointmentID), amount(_amount) {
            status = dupStr(_status);
            date   = dupStr(_date);
        }

        Bill(const Bill& obj)
            : id(obj.id), patientID(obj.patientID),
            appointmentID(obj.appointmentID), amount(obj.amount) {
            status = dupStr(obj.status);
            date   = dupStr(obj.date);
        }

        Bill& operator=(const Bill& obj) {
            if (this == &obj) return *this;
            id            = obj.id;
            patientID     = obj.patientID;
            appointmentID = obj.appointmentID;
            amount        = obj.amount;
            delete[] status; status = dupStr(obj.status);
            delete[] date;   date   = dupStr(obj.date);
            return *this;
        }

        ~Bill() {
            delete[] status;
            delete[] date;
        }

        friend ostream& operator<<(ostream& out, const Bill& obj) {
            out << "Bill ID: " << obj.id
                << " | Appointment ID: "<< obj.appointmentID
                << " | Amount: PKR " << obj.amount
                << " | Status: " << obj.status
                << " | Date: " << obj.date;
            return out;
        }

        void writeToFile(ofstream& file) const {
            file << id << ","
                << patientID << ","
                << appointmentID << ","
                << amount << ","
                << status << ","
                << date;
        }

        // Getters
        int getID() const { 
            return id; 
        }
        int getPatientID() const { 
            return patientID; 
        }
        int getAppointmentID() const { 
            return appointmentID; 
        }
        double getAmount() const { 
            return amount; 
        }
        const char* getStatus() const { 
            return status; 
        }
        const char* getDate() const { 
            return date; 
        }

        // Status
        bool isUnpaid() const { 
            return strEq(status, "unpaid");    
        }
        bool isPaid() const { 
            return strEq(status, "paid");      
        }
        bool isCancelled() const { 
            return strEq(status, "cancelled"); 
        }


        void setStatus(const char* newStatus) {
            delete[] status;
            status = dupStr(newStatus);
        }

        static double totalUnpaid(Bill** bills, int count) {
            double total = 0.0;
            for (int i = 0; i < count; i++) {
                if (bills[i]->isUnpaid()) {
                    total += bills[i]->getAmount();
                }
            }
            return total;
        }
};

#endif