#include "Header/admin.h"
#include "Header/Doctor.h"
#include "Header/Patient.h"
#include "Header/Appointment.h"
#include "Header/Bill.h"
#include "Header/Prescription.h"
#include "Header/Storage.h"
#include "Header/FileHandler.h"
#include "Header/Validator.h"
#include "Header/Exceptions.h"
#include "Header/constants.h"

static bool strEq(const char* a, const char* b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0');
}

// Writes today's date into buf as "DD-MM-YYYY". buf needs at least 11 bytes.
static void getTodayDate(char* buf) {
    time_t now = time(0);
    tm* localTime = localtime(&now);
    strftime(buf, 11, "%d-%m-%Y", localTime);
}

// Converts "DD-MM-YYYY" to integer YYYYMMDD for chronological comparison.
static int dateToInt(const char* date) {
    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100
              + (date[8] - '0') * 10   + (date[9] - '0');

    return year * 10000 + month * 100 + day;
}

// Returns true when a bill date is strictly more than 7 days before today.
static bool isOverdue(const char* dateStr) {
    tm billTime = {};
    billTime.tm_mday = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
    billTime.tm_mon = (dateStr[3] - '0') * 10 + (dateStr[4] - '0') - 1;
    billTime.tm_year = (dateStr[6] - '0') * 1000 + (dateStr[7] - '0') * 100
                      + (dateStr[8] - '0') * 10   + (dateStr[9] - '0') - 1900;
    billTime.tm_hour = 0;
    billTime.tm_min = 0;
    billTime.tm_sec = 0;
    billTime.tm_isdst = -1;
    time_t billTimeT = mktime(&billTime);
    time_t now = time(0);
    return difftime(now, billTimeT) > 7.0 * 24.0 * 3600.0;
}

// Returns max existing doctor ID + 1.
static int nextDoctorID(Storage<Doctor>& docStorage) {
    int maxID = 0;
    for (int i = 0; i < docStorage.size(); i++)
        if (docStorage.getAll()[i].getID() > maxID)
            maxID = docStorage.getAll()[i].getID();
    return maxID + 1;
}

// Returns max existing patient ID + 1.
static int nextPatientID(Storage<Patient>& patStorage) {
    int maxID = 0;
    for (int i = 0; i < patStorage.size(); i++)
        if (patStorage.getAll()[i].getID() > maxID)
            maxID = patStorage.getAll()[i].getID();
    return maxID + 1;
}

// Returns true when the given doctor has at least one pending appointment.
static bool doctorHasPending(int doctorID, Storage<Appointment>& appointments) {
    for (int i = 0; i < appointments.size(); i++)
        if (appointments.getAll()[i].getDoctorID() == doctorID &&
            appointments.getAll()[i].isPending()) return true;
    return false;
}

// Returns true when the given patient has at least one pending appointment.
static bool patientHasPending(int patientID, Storage<Appointment>& appointments) {
    for (int i = 0; i < appointments.size(); i++)
        if (appointments.getAll()[i].getPatientID() == patientID &&
            appointments.getAll()[i].isPending()) return true;
    return false;
}

// Returns true when the given patient has at least one unpaid bill.
static bool patientHasUnpaidBills(int patientID, Storage<Bill>& bills) {
    for (int i = 0; i < bills.size(); i++)
        if (bills.getAll()[i].getPatientID() == patientID &&
            bills.getAll()[i].isUnpaid()) return true;
    return false;
}

Admin::Admin()
    : Person(),
      patStorage(nullptr), docStorage(nullptr), apptStorage(nullptr),
      billStorage(nullptr), prescStorage(nullptr) {}

Admin::Admin(int id, char* name, char* password)
    : Person(id, name, password),
      patStorage(nullptr), docStorage(nullptr), apptStorage(nullptr),
      billStorage(nullptr), prescStorage(nullptr) {}

Admin::Admin(const Admin& other)
    : Person(other),
      patStorage(other.patStorage), docStorage(other.docStorage),
      apptStorage(other.apptStorage), billStorage(other.billStorage),
      prescStorage(other.prescStorage) {}

Admin& Admin::operator=(const Admin& other) {
    if (this == &other) return *this;
    Person::operator=(other);
    patStorage   = other.patStorage;
    docStorage   = other.docStorage;
    apptStorage  = other.apptStorage;
    billStorage  = other.billStorage;
    prescStorage = other.prescStorage;
    return *this;
}

Admin::~Admin() {
    // Storage pointers are non-owning; do NOT delete them here.
}

void Admin::display() {
    cout << "Admin ID: " << id << " | Name: " << name << endl;
}

void Admin::writeToFile(ofstream& file) const {
    file << id << "," << name << "," << password;
}

void Admin::setStorages(
    Storage<Patient>*      pat,
    Storage<Doctor>*       doc,
    Storage<Appointment>*  appt,
    Storage<Bill>*         bill,
    Storage<Prescription>* presc
) {
    patStorage   = pat;
    docStorage   = doc;
    apptStorage  = appt;
    billStorage  = bill;
    prescStorage = presc;
}

// Prompts for doctor details, validates each field, and adds the doctor to storage.
void Admin::addDoctor() {

    char   name[51]       = {};
    char   specialization[51] = {};
    char   contactStr[12] = {};
    char   password[31]   = {};
    double fee            = 0.0;

    cin.ignore(1000, '\n');

    cout << "Enter doctor name (max 50 chars): ";
    cin.getline(name, 51);

    cout << "Enter specialization (max 50 chars): ";
    cin.getline(specialization, 51);

    cout << "Enter contact (11 digits, all numeric): ";
    cin >> contactStr;
    if (!Validator::validateContact(contactStr)) {
        cout << "Invalid contact. Must be exactly 11 numeric digits." << endl;
        return;
    }

    cout << "Enter password (min 6 chars): ";
    cin >> password;
    if (!Validator::validatePassword(password)) {
        cout << "Invalid password. Must be at least 6 characters." << endl;
        return;
    }

    cout << "Enter consultation fee (PKR): ";
    cin >> fee;
    if (!Validator::validatePositiveFloat(fee)) {
        cout << "Invalid fee. Must be a positive number." << endl;
        return;
    }

    int contactDigits[11];
    for (int i = 0; i < 11; i++) contactDigits[i] = contactStr[i] - '0';

    int newID = nextDoctorID(*docStorage);
    Doctor newDoctor(newID, name, password, specialization, contactDigits, fee);
    docStorage->add(newDoctor);
    fileHandler::appendDoctor(newDoctor);

    cout << "Doctor added successfully. ID: " << newID << "." << endl;
}

// Lists all doctors, prompts for an ID, and removes the matching doctor if no pending appointments exist.
void Admin::removeDoctor() {

    cout << "=== All Doctors ===" << endl;
    for (int i = 0; i < docStorage->size(); i++) {
        Doctor& doctor = docStorage->getAll()[i];
        cout << "ID: " << doctor.getID()
             << " | Name: " << doctor.getName()
             << " | Specialization: " << doctor.getSpecialization()
             << " | Fee: PKR " << doctor.getFee() << endl;
    }

    int doctorID;
    cout << "Enter Doctor ID to remove: ";
    cin >> doctorID;

    if (docStorage->findByID(doctorID) == nullptr) {
        cout << "Doctor not found." << endl;
        return;
    }

    if (doctorHasPending(doctorID, *apptStorage)) {
        cout << "Cannot remove doctor with pending appointments. "
             << "Cancel or reassign them first." << endl;
        return;
    }

    docStorage->removeByID(doctorID);
    fileHandler::saveAllDoctors(*docStorage);

    cout << "Doctor removed." << endl;
}

// Prompts for patient details, validates each field, and adds the patient to storage.
void Admin::addPatient() {

    char name[51] = {};
    char contactStr[12] = {};
    char password[31] = {};
    int age = 0;
    char gender = '\0';
    double balance = 0.0;

    cin.ignore(1000, '\n');

    cout << "Enter patient name (max 50 chars): ";
    cin.getline(name, 51);

    cout << "Enter age: ";
    cin >> age;
    if (!Validator::validateAge(age)) {
        cout << "Invalid age. Must be between 1 and 120." << endl;
        return;
    }

    cout << "Enter gender (M/F): ";
    cin >> gender;
    if (gender == 'm') gender = 'M';
    if (gender == 'f') gender = 'F';
    if (!Validator::validateGender(gender)) {
        cout << "Invalid gender. Enter M or F." << endl;
        return;
    }

    cout << "Enter contact (11 digits): ";
    cin >> contactStr;
    if (!Validator::validateContact(contactStr)) {
        cout << "Invalid contact. Must be exactly 11 numeric digits." << endl;
        return;
    }

    cout << "Enter password (min 6 chars): ";
    cin >> password;
    if (!Validator::validatePassword(password)) {
        cout << "Invalid password. Must be at least 6 characters." << endl;
        return;
    }

    cout << "Enter initial balance (PKR): ";
    cin >> balance;
    if (!Validator::validatePositiveFloat(balance)) {
        cout << "Balance must be a positive number." << endl;
        return;
    }

    int contactDigits[11];
    for (int i = 0; i < 11; i++) contactDigits[i] = contactStr[i] - '0';

    int newID = nextPatientID(*patStorage);
    Patient newPatient(newID, name, password, age, gender, contactDigits, balance);
    patStorage->add(newPatient);
    fileHandler::appendPatient(newPatient);

    cout << "Patient added successfully. ID: " << newID << "." << endl;
}

// Lists all patients, prompts for an ID, and removes the patient if no pending appointments or unpaid bills exist.
void Admin::removePatient() {

    cout << "=== All Patients ===" << endl;
    for (int i = 0; i < patStorage->size(); i++) {
        Patient& patient = patStorage->getAll()[i];
        int* contactDigits = patient.getContact();
        cout << "ID: " << patient.getID()
             << " | Name: " << patient.getName()
             << " | Age: " << patient.getAge()
             << " | Gender: " << patient.getGender()
             << " | Contact: ";
        for (int j = 0; j < 11; j++) cout << contactDigits[j];
        cout << " | Balance: PKR " << patient.getBalance() << endl;
    }

    int patientID;
    cout << "Enter Patient ID to remove: ";
    cin >> patientID;

    if (patStorage->findByID(patientID) == nullptr) {
        cout << "Patient not found." << endl;
        return;
    }

    if (patientHasPending(patientID, *apptStorage)) {
        cout << "Cannot remove patient with pending appointments." << endl;
        return;
    }

    if (patientHasUnpaidBills(patientID, *billStorage)) {
        cout << "Cannot remove patient with unpaid bills." << endl;
        return;
    }

    fileHandler::deletePatientByID(
        patientID, *patStorage, *apptStorage, *billStorage, *prescStorage
    );
    fileHandler::saveAllPatients     (*patStorage);
    fileHandler::saveAllAppointments (*apptStorage);
    fileHandler::saveAllBills        (*billStorage);
    fileHandler::saveAllPrescriptions(*prescStorage);

    cout << "Patient removed successfully." << endl;
}

// Displays all patients with their contact info, balance, and unpaid bill count.
void Admin::viewAllPatients() {

    if (patStorage->size() == 0) {
        cout << "No patients found." << endl;
        return;
    }

    cout << "=== All Patients ===" << endl;

    for (int i = 0; i < patStorage->size(); i++) {
        Patient& patient = patStorage->getAll()[i];

        int unpaidCount = 0;
        for (int j = 0; j < billStorage->size(); j++)
            if (billStorage->getAll()[j].getPatientID() == patient.getID() &&
                billStorage->getAll()[j].isUnpaid()) unpaidCount++;

        int* contactDigits = patient.getContact();
        cout << "ID: " << patient.getID()
             << " | Name: " << patient.getName()
             << " | Age: " << patient.getAge()
             << " | Gender: " << patient.getGender()
             << " | Contact: ";

        for (int j = 0; j < 11; j++) cout << contactDigits[j];
        cout << " | Balance: PKR "   << patient.getBalance()
             << " | Unpaid Bills: "  << unpaidCount
             << endl;
    }
}

// Displays all doctors with their contact info and consultation fee.
void Admin::viewAllDoctors() {

    if (docStorage->size() == 0) {
        cout << "No doctors found." << endl;
        return;
    }

    cout << "=== All Doctors ===" << endl;

    for (int i = 0; i < docStorage->size(); i++) {
        Doctor& doctor = docStorage->getAll()[i];
        int* contactDigits = doctor.getContact();
        cout << "ID: " << doctor.getID()
             << " | Name: " << doctor.getName()
             << " | Specialization: " << doctor.getSpecialization()
             << " | Contact: ";
        for (int j = 0; j < 11; j++) cout << contactDigits[j];
        cout << " | Fee: PKR " << doctor.getFee() << endl;
    }
}

// Displays all appointments sorted by date descending (most recent first).
void Admin::viewAllAppointments() {

    int total = apptStorage->size();
    if (total == 0) {
        cout << "No appointments found." << endl;
        return;
    }

    Appointment** sorted = new Appointment*[total];
    for (int i = 0; i < total; i++)
        sorted[i] = &apptStorage->getAll()[i];

    // Bubble sort date descending
    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - i - 1; j++) {
            if (dateToInt(sorted[j]->getDate()) <
                dateToInt(sorted[j + 1]->getDate())) {
                Appointment* temp = sorted[j];
                sorted[j]         = sorted[j + 1];
                sorted[j + 1]     = temp;
            }
        }
    }

    cout << "=== All Appointments (most recent first) ===" << endl;

    for (int i = 0; i < total; i++) {
        Appointment* appointment = sorted[i];

        const char* patientName = "Unknown";
        const char* doctorName  = "Unknown";
        Patient* patient = patStorage->findByID(appointment->getPatientID());
        Doctor*  doctor  = docStorage->findByID(appointment->getDoctorID());
        if (patient) patientName = patient->getName();
        if (doctor)  doctorName  = doctor->getName();

        cout << "ID: " << appointment->getID()
             << " | Patient: " << patientName
             << " | Doctor: " << doctorName
             << " | Date: " << appointment->getDate()
             << " | Time Slot: " << appointment->getTimeSlot()
             << " | Status: " << appointment->getStatus()
             << endl;
    }

    delete[] sorted;
}

// Displays all unpaid bills, marking those more than 7 days old as overdue.
void Admin::viewUnpaidBills() {

    bool found = false;
    cout << "=== Unpaid Bills ===" << endl;

    for (int i = 0; i < billStorage->size(); i++) {
        Bill& bill = billStorage->getAll()[i];
        if (!bill.isUnpaid()) continue;

        found = true;

        const char* patientName = "Unknown";
        Patient* patient = patStorage->findByID(bill.getPatientID());
        if (patient) patientName = patient->getName();

        cout << "Bill ID: "       << bill.getID()
             << " | Patient: "    << patientName
             << " | Amount: PKR " << bill.getAmount()
             << " | Date: "       << bill.getDate();

        if (isOverdue(bill.getDate())) cout << " [OVERDUE]";

        cout << endl;
    }

    if (!found) cout << "No unpaid bills found." << endl;
}

// Archives the patient to discharged.txt then removes them from all storage if no unpaid bills or pending appointments.
void Admin::dischargePatient() {

    int patientID;
    cout << "Enter Patient ID: ";
    cin >> patientID;

    Patient* patient = patStorage->findByID(patientID);
    if (patient == nullptr) {
        cout << "Patient not found." << endl;
        return;
    }

    if (patientHasUnpaidBills(patientID, *billStorage)) {
        cout << "Cannot discharge patient with unpaid bills." << endl;
        return;
    }
    if (patientHasPending(patientID, *apptStorage)) {
        cout << "Cannot discharge patient with pending appointments." << endl;
        return;
    }

    {
        ofstream archiveFile("discharged.txt", ios::app);
        if (archiveFile.is_open()) {
            patient->writeToFile(archiveFile);
            archiveFile << "\n";
            archiveFile.close();
        }
    }

    fileHandler::deletePatientByID(
        patientID, *patStorage, *apptStorage, *billStorage, *prescStorage
    );

    fileHandler::saveAllPatients     (*patStorage);
    fileHandler::saveAllAppointments (*apptStorage);
    fileHandler::saveAllBills        (*billStorage);
    fileHandler::saveAllPrescriptions(*prescStorage);

    cout << "Patient discharged and archived successfully." << endl;
}

// Reads and prints all entries from security_log.txt.
void Admin::viewSecurityLog() {

    ifstream logFile("security_log.txt");
    if (!logFile.is_open()) {
        cout << "No security events logged." << endl;
        return;
    }

    char line[300];
    bool isEmpty = true;

    cout << "=== Security Log ===" << endl;

    while (logFile.getline(line, 300)) {
        int len = 0;
        while (line[len] != '\0') len++;
        if (len == 0) continue;

        cout << line << endl;
        isEmpty = false;
    }
    logFile.close();

    if (isEmpty) cout << "No security events logged." << endl;
}

// Prints today's appointment counts, revenue, patients with outstanding bills, and a per-doctor summary.
void Admin::generateDailyReport() {

    char today[11];
    getTodayDate(today);

    cout << "=== Daily Report: " << today << " ===" << endl;

    int totalCount = 0, pendingCount = 0, completedCount = 0;
    int noShowCount = 0, cancelledCount = 0;

    for (int i = 0; i < apptStorage->size(); i++) {

        Appointment& appt = apptStorage->getAll()[i];
        if (!strEq(appt.getDate(), today)) continue;

        totalCount++;

        if (appt.isPending()) pendingCount++;
        else if (appt.isCompleted()) completedCount++;
        else if (strEq(appt.getStatus(), "no-show")) noShowCount++;
        else if (appt.isCancelled()) cancelledCount++;
    }

    cout << "Total appointments today: " << totalCount
         << " (Pending: " << pendingCount
         << " Completed: " << completedCount
         << " No-show: " << noShowCount
         << " Cancelled: " << cancelledCount
         << ")" << endl;

    double revenueToday = 0.0;
    for (int i = 0; i < billStorage->size(); i++) {
        Bill& bill = billStorage->getAll()[i];
        if (bill.isPaid() && strEq(bill.getDate(), today)) revenueToday += bill.getAmount();
    }
    cout << "Revenue collected today (paid bills): PKR " << revenueToday << endl;

    cout << "Patients with outstanding unpaid bills:" << endl;
    bool anyUnpaid = false;
    
    for (int i = 0; i < patStorage->size(); i++) {
        
        Patient& patient = patStorage->getAll()[i];
        double amountOwed = 0.0;

        for (int j = 0; j < billStorage->size(); j++) {
            Bill& bill = billStorage->getAll()[j];
            if (bill.getPatientID() == patient.getID() && bill.isUnpaid())
                amountOwed += bill.getAmount();
        }
        if (amountOwed > 0.0) {
            anyUnpaid = true;
            cout << "  " << patient.getName() << " | PKR " << amountOwed << endl;
        }
    }
    if (!anyUnpaid) cout << "  None." << endl;

    cout << "Doctor-wise summary for today:" << endl;

    for (int i = 0; i < docStorage->size(); i++) {
        Doctor& doctor = docStorage->getAll()[i];

        int doctorCompleted = 0, doctorPending = 0, doctorNoShow = 0;

        for (int j = 0; j < apptStorage->size(); j++) {
            Appointment& appt = apptStorage->getAll()[j];

            if (appt.getDoctorID() != doctor.getID()) continue;
            if (!strEq(appt.getDate(), today)) continue;

            if (appt.isCompleted()) doctorCompleted++;
            else if (appt.isPending()) doctorPending++;
            else if (strEq(appt.getStatus(), "no-show")) doctorNoShow++;
        }
        cout << "  " << doctor.getName()
             << " | Completed: " << doctorCompleted
             << " | Pending: " << doctorPending
             << " | No-show: " << doctorNoShow
             << endl;
    }
}