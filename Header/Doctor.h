#ifndef DOCTOR_H
#define DOCTOR_H

#include "constants.h"
#include "Person.h"
#include "Appointment.h"
#include "Patient.h"
#include "Prescription.h"
#include "Bill.h"
#include "Storage.h"

template<class T> class Storage;
class Appointment;
class Patient;
class Prescription;
class Bill;

class Doctor : public Person {
    private:
        char* specialization;
        int* contact;
        double fee;

        Storage<Appointment>* apptStorage;
        Storage<Patient>* patStorage;
        Storage<Prescription>* prescStorage;
        Storage<Bill>* billStorage;

    public:


        Doctor()
            : Person(), fee(0.0),
            apptStorage(nullptr), patStorage(nullptr),
            prescStorage(nullptr), billStorage(nullptr) {
            specialization    = new char[1]; specialization[0] = '\0';
            contact           = new int[11];
            for (int i = 0; i < 11; i++) contact[i] = 0;
        }

        Doctor(int _id, char* _name, char* _password,
            char* _specialization, int* _contact, double _fee)
            : Person(_id, _name, _password), fee(_fee),
            apptStorage(nullptr), patStorage(nullptr),
            prescStorage(nullptr), billStorage(nullptr) {
            int sz = lenArr(_specialization);
            specialization = new char[sz + 1];
            for (int i = 0; i <= sz; i++) specialization[i] = _specialization[i];

            contact = new int[11];
            for (int i = 0; i < 11; i++) contact[i] = _contact[i];
        }

        Doctor(const Doctor& obj)
            : Person(obj), fee(obj.fee),
            apptStorage(obj.apptStorage), patStorage(obj.patStorage),
            prescStorage(obj.prescStorage), billStorage(obj.billStorage) {
            int sz = lenArr(obj.specialization);
            specialization = new char[sz + 1];
            for (int i = 0; i <= sz; i++) specialization[i] = obj.specialization[i];

            contact = new int[11];
            for (int i = 0; i < 11; i++) contact[i] = obj.contact[i];
        }

        Doctor& operator=(const Doctor& obj) {
            if (this == &obj) return *this;
            Person::operator=(obj);
            fee          = obj.fee;
            apptStorage  = obj.apptStorage;
            patStorage   = obj.patStorage;
            prescStorage = obj.prescStorage;
            billStorage  = obj.billStorage;

            delete[] specialization;
            int sz = lenArr(obj.specialization);
            specialization = new char[sz + 1];
            for (int i = 0; i <= sz; i++) specialization[i] = obj.specialization[i];

            delete[] contact;
            contact = new int[11];
            for (int i = 0; i < 11; i++) contact[i] = obj.contact[i];

            return *this;
        }

        // Two doctors are equal when they share the same ID
        bool operator==(const Doctor& other) const {
            return id == other.id;
        }

        virtual ~Doctor() {
            delete[] specialization;
            delete[] contact;
        }

        friend ostream& operator<<(ostream& out, const Doctor& obj) {
            out << "ID: " << obj.id
                << " | Name: " << obj.name
                << " | Specialization: " << obj.specialization
                << " | Contact: ";
            for (int i = 0; i < 11; i++) out << obj.contact[i];

            out << " | Fee: PKR " << obj.fee;

            return out;
        }

        void display() override {
            cout << *this << endl;
        }

        void writeToFile(ofstream& file) const {
            file << id << "," << name << "," << specialization << ",";
            for (int i = 0; i < 11; i++) file << contact[i];
            file << "," << password << "," << fee;
        }

        void setStorages(
            Storage<Appointment>* a,
            Storage<Patient>* p,
            Storage<Prescription>* pr,
            Storage<Bill>* b
        ) {
            apptStorage  = a;
            patStorage   = p;
            prescStorage = pr;
            billStorage  = b;
        }

        // Getters
        int getID() const { 
            return id;
        }
        char* getName() const { 
            return name; 
        }
        char* getSpecialization() const { 
            return specialization; 
        }
        int* getContact() const { 
            return contact; 
        }
        double getFee() const { 
            return fee; 
        }

        void setFee(double newFee) { 
            fee = newFee; 
        }

        // Doctor actions
        void viewTodaysAppointments();
        bool markAppointmentComplete(int appointmentID);
        bool markAppointmentNoShow(int appointmentID);
        bool writePrescription(int appointmentID, char* medicines, char* notes);
        void viewPatientMedicalHistory(int patientID);
};
#endif