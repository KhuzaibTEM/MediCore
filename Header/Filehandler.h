#ifndef FILE_H
#define FILE_H

#include "constants.h"
#include <cstring>
#include "Storage.h"
#include "Patient.h"
#include "Doctor.h"
#include "admin.h"
#include "appointment.h"
#include "Prescription.h"
#include "Bill.h"
#include "Exceptions.h"

class fileHandler {
    public:

        static void loadPatients(Storage<Patient>& patients) {

            ifstream file("patients.txt");
            if (!file.is_open()) {
                throw FileNotFoundException("patients.txt");
            }
            char line[300];
            file.getline(line, 300); // skip header

            while (file.getline(line, 300)) {
                char* token;

                token = strtok(line, ",");
                int id = atoi(token);

                token = strtok(NULL, ",");
                char* name = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) name[i] = token[i];

                token = strtok(NULL, ",");
                int age = atoi(token);

                token = strtok(NULL, ",");
                char gender = token[0];

                token = strtok(NULL, ",");
                int* contact = new int[11];
                for (int i = 0; i < 11; i++) contact[i] = token[i] - '0';

                token = strtok(NULL, ",");
                char* password = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) password[i] = token[i];

                token = strtok(NULL, ",");
                double balance = atof(token);

                Patient p(id, name, password, age, gender, contact, balance);
                patients.add(p);

                delete[] name;
                delete[] contact;
                delete[] password;
            }
            file.close();
        }

        static void loadDoctors(Storage<Doctor>& doctors) {

            ifstream file("doctors.txt");
            if (!file.is_open()) {
                throw FileNotFoundException("doctors.txt");
            }
            char* line = new char[300];
            file.getline(line, 300);

            while (file.getline(line, 300)) {
                char* token;

                token = strtok(line, ",");
                int id = atoi(token);

                token = strtok(NULL, ",");
                char* name = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) name[i] = token[i];

                token = strtok(NULL, ",");
                char* specialization = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) specialization[i] = token[i];

                token = strtok(NULL, ",");
                int* contact = new int[11];
                for (int i = 0; i < 11; i++) contact[i] = token[i] - '0';

                token = strtok(NULL, ",");
                char* password = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) password[i] = token[i];

                token = strtok(NULL, ",");
                double fee = atof(token);

                Doctor d(id, name, password, specialization, contact, fee);
                doctors.add(d);

                delete[] name;
                delete[] specialization;
                delete[] contact;
                delete[] password;
            }
            delete[] line;
            file.close();
        }

        /*
         * appointments.txt
         * Header: appointment_id,patient_id,doctor_id,date,time_slot,status
         */
        static void loadAppointments(Storage<Appointment>& appointments) {

            ifstream file("appointments.txt");
            if (!file.is_open()) {
                throw FileNotFoundException("appointments.txt");
            }
            char line[300];
            file.getline(line, 300); // skip header

            while (file.getline(line, 300)) {
                char* token;

                token = strtok(line, ",");
                int id = atoi(token);

                token = strtok(NULL, ",");
                int patientID = atoi(token);

                token = strtok(NULL, ",");
                int doctorID = atoi(token);

                token = strtok(NULL, ",");
                char* date = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) date[i] = token[i];

                token = strtok(NULL, ",");
                char* timeSlot = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) timeSlot[i] = token[i];

                token = strtok(NULL, ",");
                char* status = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) status[i] = token[i];

                Appointment a(id, patientID, doctorID, date, timeSlot, status);
                appointments.add(a);

                delete[] date;
                delete[] timeSlot;
                delete[] status;
            }
            file.close();
        }

        /*
         * prescriptions.txt
         * Header: prescription_id,appointment_id,patient_id,
         *         doctor_id,date,medicines,notes
         */
        static void loadPrescriptions(Storage<Prescription>& prescriptions) {

            ifstream file("prescriptions.txt");
            if (!file.is_open()) {
                throw FileNotFoundException("prescriptions.txt");
            }
            char line[1024]; // medicines field can be large
            file.getline(line, 1024);

            while (file.getline(line, 1024)) {
                char* token;

                token = strtok(line, ",");
                int id = atoi(token);

                token = strtok(NULL, ",");
                int appointmentID = atoi(token);

                token = strtok(NULL, ",");
                int patientID = atoi(token);

                token = strtok(NULL, ",");
                int doctorID = atoi(token);

                token = strtok(NULL, ",");
                char* date = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) date[i] = token[i];

                token = strtok(NULL, ",");
                char* medicines = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) medicines[i] = token[i];

                token = strtok(NULL, ",");
                char* notes = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) notes[i] = token[i];

                Prescription p(id, appointmentID, patientID, doctorID,
                                date, medicines, notes);
                prescriptions.add(p);

                delete[] date;
                delete[] medicines;
                delete[] notes;
            }
            file.close();
        }

        /*
         * bills.txt
         * Header: bill_id,patient_id,appointment_id,amount,status,date
         */
        static void loadBills(Storage<Bill>& bills) {

            ifstream file("bills.txt");
            if (!file.is_open()) {
                throw FileNotFoundException("bills.txt");
            }
            char line[300];
            file.getline(line, 300);

            while (file.getline(line, 300)) {
                char* token;

                token = strtok(line, ",");
                int id = atoi(token);

                token = strtok(NULL, ",");
                int patientID = atoi(token);

                token = strtok(NULL, ",");
                int appointmentID = atoi(token);

                token = strtok(NULL, ",");
                double amount = atof(token);

                token = strtok(NULL, ",");
                char* status = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) status[i] = token[i];

                token = strtok(NULL, ",");
                char* date = new char[lenArr(token) + 1];
                for (int i = 0; i <= lenArr(token); i++) date[i] = token[i];

                Bill b(id, patientID, appointmentID, amount, status, date);
                bills.add(b);

                delete[] status;
                delete[] date;
            }
            file.close();
        }

        // Append
        static void appendPatient(const Patient& p) {
            ofstream file("patients.txt", ios::app);
            file << "\n";
            p.writeToFile(file);
            file.close();
        }

        static void appendDoctor(const Doctor& d) {
            ofstream file("doctors.txt", ios::app);
            file << "\n";
            d.writeToFile(file);
            file.close();
        }

        static void appendAppointment(const Appointment& a) {
            ofstream file("appointments.txt", ios::app);
            file << "\n";
            a.writeToFile(file);
            file.close();
        }

        static void appendPrescription(const Prescription& p) {
            ofstream file("prescriptions.txt", ios::app);
            file << "\n";
            p.writeToFile(file);
            file.close();
        }

        static void appendBill(const Bill& b) {
            ofstream file("bills.txt", ios::app);
            file << "\n";
            b.writeToFile(file);
            file.close();
        }

        //  Save-all def
        static void saveAllPatients(Storage<Patient>& patients) {
            ofstream file("patients.txt", ios::trunc);
            file << "patient_id,name,age,gender,contact,password,balance";
            for (int i = 0; i < patients.size(); i++) {
                file << "\n";
                patients.getAll()[i].writeToFile(file);
            }
            file.close();
        }

        static void saveAllDoctors(Storage<Doctor>& doctors) {
            ofstream file("doctors.txt", ios::trunc);
            file << "doctor_id,name,specialization,contact,password,fee";
            for (int i = 0; i < doctors.size(); i++) {
                file << "\n";
                doctors.getAll()[i].writeToFile(file);
            }
            file.close();
        }

        static void saveAllAppointments(Storage<Appointment>& appointments) {
            ofstream file("appointments.txt", ios::trunc);
            file << "appointment_id,patient_id,doctor_id,date,time_slot,status";
            for (int i = 0; i < appointments.size(); i++) {
                file << "\n";
                appointments.getAll()[i].writeToFile(file);
            }
            file.close();
        }

        static void saveAllPrescriptions(Storage<Prescription>& prescriptions) {
            ofstream file("prescriptions.txt", ios::trunc);
            file << "prescription_id,appointment_id,patient_id,"
                    "doctor_id,date,medicines,notes";
            for (int i = 0; i < prescriptions.size(); i++) {
                file << "\n";
                prescriptions.getAll()[i].writeToFile(file);
            }
            file.close();
        }

        static void saveAllBills(Storage<Bill>& bills) {
            ofstream file("bills.txt", ios::trunc);
            file << "bill_id,patient_id,appointment_id,amount,status,date";
            for (int i = 0; i < bills.size(); i++) {
                file << "\n";
                bills.getAll()[i].writeToFile(file);
            }
            file.close();
        }

        // Delete
        static bool deletePatientByID(
            int patientID,
            Storage<Patient>&      patients,
            Storage<Appointment>&  appointments,
            Storage<Bill>&         bills,
            Storage<Prescription>& prescriptions
        ) {
            bool removed = patients.removeByID(patientID);
            if (!removed) return false;

            for (int i = 0; i < appointments.size(); ) {
                if (appointments.getAll()[i].getPatientID() == patientID) {
                    appointments.removeByID(appointments.getAll()[i].getID());
                }
                else i++;
            }
            for (int i = 0; i < bills.size(); ) {
                if (bills.getAll()[i].getPatientID() == patientID) {
                    bills.removeByID(bills.getAll()[i].getID());
                }
                    
                else i++;
            }
            for (int i = 0; i < prescriptions.size(); ) {
                if (prescriptions.getAll()[i].getPatientID() == patientID) {
                    prescriptions.removeByID(prescriptions.getAll()[i].getID());
                }
                    
                else i++;
            }
            return true;
        }


        // Update def
        template <class T>
        static bool updateRecordByID(
            Storage<T>& storage,
            int id,
            const T& updatedObj
        ) {
            T* obj = storage.findByID(id);
            if (obj == nullptr) return false;
            *obj = updatedObj;
            return true;
        }
};

#endif