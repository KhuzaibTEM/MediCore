#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include "constants.h"
#include <fstream>
#include <iostream>
using namespace std;

class Appointment {
    private:
        int   id;
        int   patientID;
        int   doctorID;
        char* date;       // format: DD-MM-YYYY
        char* timeSlot;   // format: HH:MM  (one of 8 fixed daily slots)
        char* status;     // "pending" | "cancelled" | "completed"


        static char* dupStr(const char* src) {
            int len = lenArr(src);
            char* dst = new char[len + 1];
            for (int i = 0; i <= len; i++) dst[i] = src[i];
            return dst;
        }

        // Character-by-character equality check
        static bool strEq(const char* a, const char* b) {
            int i = 0;
            while (a[i] != '\0' && b[i] != '\0') {
                if (a[i] != b[i]) return false;
                i++;
            }
            return (a[i] == '\0' && b[i] == '\0');
        }

    public:

        Appointment() : id(0), patientID(0), doctorID(0) {
            date = new char[1]; date[0] = '\0';
            timeSlot = new char[1]; timeSlot[0] = '\0';
            status = new char[1]; status[0] = '\0';
        }

        Appointment(int _id, int _patientID, int _doctorID,
        const char* _date, const char* _timeSlot,
        const char* _status)
            : id(_id), patientID(_patientID), doctorID(_doctorID) {
            date = dupStr(_date);
            timeSlot = dupStr(_timeSlot);
            status = dupStr(_status);
        }

        Appointment(const Appointment& obj)
            : id(obj.id), patientID(obj.patientID), doctorID(obj.doctorID) {
            date = dupStr(obj.date);
            timeSlot = dupStr(obj.timeSlot);
            status = dupStr(obj.status);
        }

        Appointment& operator=(const Appointment& obj) {
            if (this == &obj) return *this;
            id = obj.id;
            patientID = obj.patientID;
            doctorID = obj.doctorID;

            delete[] date;     
            date = dupStr(obj.date);

            delete[] timeSlot; 
            timeSlot = dupStr(obj.timeSlot);
            
            delete[] status;   
            status = dupStr(obj.status);

            return *this;
        }

        ~Appointment() {
            delete[] date;
            delete[] timeSlot;
            delete[] status;
        }

        bool operator==(const Appointment& other) const {
            return  doctorID       == other.doctorID
                &&  strEq(date,         other.date)
                &&  strEq(timeSlot,     other.timeSlot)
                && !strEq(status,       "cancelled")
                && !strEq(other.status, "cancelled");
        }

        bool operator!=(const Appointment& other) const {
            return !(*this == other);
        }

        friend ostream& operator<<(ostream& out, const Appointment& obj) {
            out << "Appointment ID: " << obj.id
                << " | Patient ID: "  << obj.patientID
                << " | Doctor ID: "   << obj.doctorID
                << " | Date: "        << obj.date
                << " | Time Slot: "   << obj.timeSlot
                << " | Status: "      << obj.status;
            return out;
        }

        // File I/O
        void writeToFile(ofstream& file) const {
            file << id        << ","
                << patientID << ","
                << doctorID  << ","
                << date      << ","
                << timeSlot  << ","
                << status;
        }

        // Getters 

        int getID() const { 
            return id; 
        }
        int getPatientID() const { 
            return patientID; 
        }
        int getDoctorID()  const { 
            return doctorID; 
        }
        const char* getDate() const { 
            return date; 
        }
        const char* getTimeSlot() const { 
            return timeSlot; 
        }
        const char* getStatus() const { 
            return status; 
        }

        // Status
        bool isPending() const { 
            return strEq(status, "pending");   
        }
        bool isCancelled() const { 
            return strEq(status, "cancelled"); 
        }
        bool isCompleted() const { 
            return strEq(status, "completed"); 
        }

        void setStatus(const char* newStatus) {
            delete[] status;
            status = dupStr(newStatus);
        }
};

#endif