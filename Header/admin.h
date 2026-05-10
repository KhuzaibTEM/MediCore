#ifndef ADMIN_H
#define ADMIN_H

#include "Person.h"
#include "constants.h"
#include <fstream>
#include <iostream>
using namespace std;

template<class T> class Storage;
class Patient;
class Doctor;
class Appointment;
class Bill;
class Prescription;

class Admin : public Person {
    private:

        Storage<Patient>*      patStorage;
        Storage<Doctor>*       docStorage;
        Storage<Appointment>*  apptStorage;
        Storage<Bill>*         billStorage;
        Storage<Prescription>* prescStorage;

    public:

        Admin();

        Admin(int _id, char* _name, char* _password);

        Admin(const Admin& obj);

        Admin& operator=(const Admin& obj);

        ~Admin() override;

        /*
        * Console display: "Admin ID: [id] | Name: [name]"
        */
        void display() override;

        /*
        * Format: admin_id,name,password  (no leading/trailing newline)
        * Matches admin.txt schema.
        */
        void writeToFile(ofstream& file) const;

        void setStorages(
            Storage<Patient>* pat,
            Storage<Doctor>* doc,
            Storage<Appointment>* appt,
            Storage<Bill>* bill,
            Storage<Prescription>* presc
        );

        int getID() const { 
            return id; 
        }

        void addDoctor();

        void removeDoctor();

        void addPatient();

        void removePatient();

        void viewAllPatients();

        void viewAllDoctors();

        void viewAllAppointments();

        void viewUnpaidBills();

        void dischargePatient();

        void viewSecurityLog();

        void generateDailyReport();
};

#endif