#ifndef PRESCRIPTION_H
#define PRESCRIPTION_H

#include "constants.h"

class Prescription {
    private:
        int    id;
        int    appointmentID;
        int    patientID;
        int    doctorID;
        char*  date;        // DD-MM-YYYY
        char*  medicines;   // semicolon-separated, max 500 chars
        char*  notes;       // plain text, max 300 chars

        static char* dupStr(const char* src) {
            int len = lenArr(src);
            char* dst = new char[len + 1];
            for (int i = 0; i <= len; i++) dst[i] = src[i];
            return dst;
        }

        static char* dupStrCapped(const char* src, int maxLen) {
            int len = lenArr(src);
            if (len >= maxLen) len = maxLen - 1;   // truncate silently
            char* dst = new char[len + 1];
            for (int i = 0; i < len; i++) dst[i] = src[i];
            dst[len] = '\0';
            return dst;
        }

    public:

        Prescription()
            : id(0), appointmentID(0), patientID(0), doctorID(0) {
            date = new char[1]; 
            date[0] = '\0';

            medicines = new char[1]; 
            medicines[0] = '\0';

            notes = new char[1]; 
            notes[0] = '\0';
        }

        Prescription(int _id, int _appointmentID, int _patientID, int _doctorID,
                    const char* _date, const char* _medicines, const char* _notes)
            : id(_id), appointmentID(_appointmentID),
            patientID(_patientID), doctorID(_doctorID) {
            date = dupStr(_date);
            medicines = dupStrCapped(_medicines, 500);
            notes = dupStrCapped(_notes,     300);
        }

        Prescription(const Prescription& obj)
            : id(obj.id), appointmentID(obj.appointmentID),
            patientID(obj.patientID), doctorID(obj.doctorID) {
            date = dupStr(obj.date);
            medicines = dupStr(obj.medicines);
            notes = dupStr(obj.notes);
        }

        Prescription& operator=(const Prescription& obj) {
            if (this == &obj) return *this;

            id = obj.id;
            appointmentID = obj.appointmentID;
            patientID = obj.patientID;
            doctorID = obj.doctorID;

            delete[] date;   
            date = dupStr(obj.date);

            delete[] medicines; 
            medicines = dupStr(obj.medicines);

            delete[] notes;     
            notes = dupStr(obj.notes);

            return *this;
        }

        ~Prescription() {
            delete[] date;
            delete[] medicines;
            delete[] notes;
        }

        friend ostream& operator<<(ostream& out, const Prescription& obj) {
            out << "Prescription ID: "  << obj.id
                << " | Appointment ID: "<< obj.appointmentID
                << " | Patient ID: " << obj.patientID
                << " | Doctor ID: " << obj.doctorID
                << " | Date: " << obj.date
                << " | Medicines: " << obj.medicines
                << " | Notes: " << obj.notes;

            return out;
        }

        void displayForPatient(const char* doctorName) const {
            cout << "Date: " << date
                << " | Doctor: " << doctorName
                << " | Medicines: " << medicines
                << " | Notes: " << notes
                << endl;
        }

        void writeToFile(ofstream& file) const {
            file << id << ","
                << appointmentID << ","
                << patientID << ","
                << doctorID << ","
                << date << ","
                << medicines << ","
                << notes;
        }

        // Getters
        int getID() const { 
            return id; 
        }
        int getAppointmentID() const { 
            return appointmentID; 
        }
        int getPatientID() const { 
            return patientID; 
        }
        int getDoctorID() const { 
            return doctorID; 
        }
        const char* getDate() const { 
            return date; 
        }
        const char* getMedicines() const { 
            return medicines; 
        }
        const char* getNotes() const { 
            return notes; 
        }
};

#endif