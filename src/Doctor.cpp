#include "Header/Doctor.h"
#include "Header/Appointment.h"
#include "Header/Patient.h"
#include "Header/Prescription.h"
#include "Header/Bill.h"
#include "Header/Storage.h"
#include "Header/FileHandler.h"
#include "Header/constants.h"

static bool strEq(const char* a, const char* b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0');
}

// Lexicographic comparison, used for HH:MM time slot strings
static bool strGt(const char* a, const char* b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] > b[i]) return true;
        if (a[i] < b[i]) return false;
        i++;
    }
    return false;
}

// Writes today's date into buf as "DD-MM-YYYY". buf must hold at least 11 bytes.
static void getTodayDate(char* buf) {
    time_t now = time(0);
    tm* localTime = localtime(&now);
    strftime(buf, 11, "%d-%m-%Y", localTime);
}

// Converts "DD-MM-YYYY" to integer YYYYMMDD for chronological comparison.
static int dateToInt(const char* date) {
    int day   = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year  = (date[6] - '0') * 1000 + (date[7] - '0') * 100
              + (date[8] - '0') * 10   + (date[9] - '0');
    return year * 10000 + month * 100 + day;
}

// Returns the next available prescription ID (max existing ID + 1).
static int nextPrescriptionID(Storage<Prescription>& prescStorage) {
    int maxID = 0;
    for (int i = 0; i < prescStorage.size(); i++) {
        int currentID = prescStorage.getAll()[i].getID();
        if (currentID > maxID) maxID = currentID;
    }
    return maxID + 1;
}

// Displays all of today's appointments for this doctor, sorted by time slot.
void Doctor::viewTodaysAppointments() {

    char today[11];
    getTodayDate(today);

    int total = apptStorage->size();
    Appointment** todaysAppointments = new Appointment*[total];
    int matchCount = 0;

    for (int i = 0; i < total; i++) {
        Appointment& appt = apptStorage->getAll()[i];
        if (appt.getDoctorID() == id && strEq(appt.getDate(), today)) {
            todaysAppointments[matchCount++] = &appt;
        }
    }

    if (matchCount == 0) {
        cout << "No appointments scheduled for today." << endl;
        delete[] todaysAppointments;
        return;
    }

    // Bubble sort ascending by time slot
    for (int i = 0; i < matchCount - 1; i++) {
        for (int j = 0; j < matchCount - i - 1; j++) {
            if (strGt(todaysAppointments[j]->getTimeSlot(), todaysAppointments[j + 1]->getTimeSlot())) {
                Appointment* temp       = todaysAppointments[j];
                todaysAppointments[j]   = todaysAppointments[j + 1];
                todaysAppointments[j + 1] = temp;
            }
        }
    }

    cout << "=== Today's Appointments (" << today << ") ===" << endl;

    for (int i = 0; i < matchCount; i++) {
        Appointment* appt = todaysAppointments[i];

        const char* patientName = "Unknown";
        Patient* patient = patStorage->findByID(appt->getPatientID());
        if (patient != nullptr) patientName = patient->getName();

        cout << "Appointment ID: " << appt->getID()
             << " | Patient: "     << patientName
             << " | Time Slot: "   << appt->getTimeSlot()
             << " | Status: "      << appt->getStatus()
             << endl;
    }

    delete[] todaysAppointments;
}

// Marks a pending appointment as completed. Appointment must belong to this doctor and be scheduled today.
bool Doctor::markAppointmentComplete(int appointmentID) {

    Appointment* appt = apptStorage->findByID(appointmentID);

    if (appt == nullptr) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }
    if (appt->getDoctorID() != id) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }
    if (!appt->isPending()) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }

    char today[11];
    getTodayDate(today);
    if (!strEq(appt->getDate(), today)) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }

    appt->setStatus("completed");
    fileHandler::saveAllAppointments(*apptStorage);

    cout << "Appointment marked as completed." << endl;
    return true;
}

// Marks a pending appointment as no-show and cancels the associated bill.
bool Doctor::markAppointmentNoShow(int appointmentID) {

    Appointment* appt = apptStorage->findByID(appointmentID);

    if (appt == nullptr) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }
    if (appt->getDoctorID() != id) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }
    if (!appt->isPending()) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }

    char today[11];
    getTodayDate(today);
    if (!strEq(appt->getDate(), today)) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }

    appt->setStatus("no-show");

    for (int i = 0; i < billStorage->size(); i++) {
        Bill& bill = billStorage->getAll()[i];
        if (bill.getAppointmentID() == appointmentID) {
            bill.setStatus("cancelled");
            break;
        }
    }

    fileHandler::saveAllAppointments(*apptStorage);
    fileHandler::saveAllBills(*billStorage);

    cout << "Appointment marked as no-show." << endl;
    return true;
}

// Writes a prescription for a completed appointment. Fails if one already exists.
bool Doctor::writePrescription(int appointmentID, char* medicines, char* notes) {

    Appointment* appt = apptStorage->findByID(appointmentID);

    if (appt == nullptr || appt->getDoctorID() != id) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }
    if (!appt->isCompleted()) {
        cout << "Invalid appointment ID." << endl;
        return false;
    }

    for (int i = 0; i < prescStorage->size(); i++) {
        if (prescStorage->getAll()[i].getAppointmentID() == appointmentID) {
            cout << "Prescription already written for this appointment." << endl;
            return false;
        }
    }

    int newPrescriptionID = nextPrescriptionID(*prescStorage);

    char today[11];
    getTodayDate(today);

    Prescription newPrescription(
        newPrescriptionID,
        appointmentID,
        appt->getPatientID(),
        id,
        today,
        medicines,
        notes
    );

    prescStorage->add(newPrescription);
    fileHandler::appendPrescription(newPrescription);

    cout << "Prescription saved." << endl;
    return true;
}

// Shows all prescriptions this doctor wrote for a patient, sorted by date descending.
// Access is denied if no completed appointment exists between this doctor and the patient.
void Doctor::viewPatientMedicalHistory(int patientID) {

    Patient* patient = patStorage->findByID(patientID);
    if (patient == nullptr) {
        cout << "Access denied. You can only view records of your own patients." << endl;
        return;
    }

    bool hasCompletedAppointment = false;
    for (int i = 0; i < apptStorage->size(); i++) {
        Appointment& appt = apptStorage->getAll()[i];
        if (appt.getPatientID() == patientID &&
            appt.getDoctorID()  == id &&
            appt.isCompleted()) {
            hasCompletedAppointment = true;
            break;
        }
    }

    if (!hasCompletedAppointment) {
        cout << "Access denied. You can only view records of your own patients." << endl;
        return;
    }

    int total = prescStorage->size();
    Prescription** matchingPrescriptions = new Prescription*[total];
    int matchCount = 0;

    for (int i = 0; i < total; i++) {
        Prescription& prescription = prescStorage->getAll()[i];
        if (prescription.getPatientID() == patientID && prescription.getDoctorID() == id) {
            matchingPrescriptions[matchCount++] = &prescription;
        }
    }

    if (matchCount == 0) {
        cout << "No prescriptions found for this patient." << endl;
        delete[] matchingPrescriptions;
        return;
    }

    // Bubble sort descending by date (most recent first)
    for (int i = 0; i < matchCount - 1; i++) {
        for (int j = 0; j < matchCount - i - 1; j++) {
            if (dateToInt(matchingPrescriptions[j]->getDate()) <
                dateToInt(matchingPrescriptions[j + 1]->getDate())) {
                Prescription* temp = matchingPrescriptions[j];
                matchingPrescriptions[j] = matchingPrescriptions[j + 1];
                matchingPrescriptions[j + 1] = temp;
            }
        }
    }

    cout << "=== Medical History for Patient: " << patient->getName() << " ===" << endl;

    for (int i = 0; i < matchCount; i++) {
        Prescription* prescription = matchingPrescriptions[i];
        cout << "Prescription ID: " << prescription->getID()
             << " | Date: " << prescription->getDate()
             << " | Medicines: " << prescription->getMedicines()
             << " | Notes: " << prescription->getNotes()
             << endl;
    }

    delete[] matchingPrescriptions;
}