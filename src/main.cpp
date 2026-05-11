// MediCore Hospital Management System

#include "constants.h"
#include "Storage.h"
#include "Patient.h"
#include "Doctor.h"
#include "admin.h"
#include "Appointment.h"
#include "Bill.h"
#include "Prescription.h"
#include "FileHandler.h"
#include "Validator.h"
#include "Exceptions.h"

// Reads an integer from cin and loops until it falls within [min, max].
static int readIntInRange(int min, int max) {
    int val;
    while (true) {
        if (cin >> val) {
            cin.ignore(1000, '\n');
            if (val >= min && val <= max) return val;
            cout << "Invalid option. Please enter " << min << "-" << max << "." << endl;
        } 
        else {
            cout << "Invalid input. Please enter a number." << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
    }
}

static bool strEq(const char* a, const char* b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0');
}

static char toLowerChar(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static bool strEqCI(const char* a, const char* b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (toLowerChar(a[i]) != toLowerChar(b[i])) return false;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0');
}

// Converts "DD-MM-YYYY" to integer YYYYMMDD for chronological comparison.
static int dateToInt(const char* date) {
    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100
              + (date[8] - '0') * 10   + (date[9] - '0');
    return year * 10000 + month * 100 + day;
}

// Writes today's date into buf as "DD-MM-YYYY". buf must hold at least 11 bytes.
static void getTodayDate(char* buf) {
    time_t now = time(0);
    tm* localTime = localtime(&now);
    strftime(buf, 11, "%d-%m-%Y", localTime);
}

// Appends a login event to security_log.txt.
static void logSecurity(const char* role, int enteredID, bool success) {
    ofstream logFile("security_log.txt", ios::app);
    if (!logFile.is_open()) return;
    char timestamp[20] = {};
    time_t now = time(0);
    strftime(timestamp, 20, "%d-%m-%Y %H:%M:%S", localtime(&now));
    logFile << timestamp << "," << role << "," << enteredID
            << "," << (success ? "SUCCESS" : "FAILED") << "\n";
    logFile.close();
}

static int nextAppointmentID(Storage<Appointment>& appointments) {
    int maxID = 0;
    for (int i = 0; i < appointments.size(); i++)
        if (appointments.getAll()[i].getID() > maxID) maxID = appointments.getAll()[i].getID();
    return maxID + 1;
}

static int nextBillID(Storage<Bill>& bills) {
    int maxID = 0;
    for (int i = 0; i < bills.size(); i++)
        if (bills.getAll()[i].getID() > maxID) maxID = bills.getAll()[i].getID();
    return maxID + 1;
}

static const char* SLOTS[8] = {
    "09:00", "10:00", "11:00", "12:00",
    "13:00", "14:00", "15:00", "16:00"
};

// Recalculates each patient's unpaid-bill count from the bills storage.
static void syncBillCounts(Storage<Patient>& patients, Storage<Bill>& bills) {
    for (int i = 0; i < patients.size(); i++) {
        int patientID = patients.getAll()[i].getID();
        int unpaidCount = 0;
        for (int j = 0; j < bills.size(); j++)
            if (bills.getAll()[j].getPatientID() == patientID &&
                bills.getAll()[j].isUnpaid()) unpaidCount++;
        patients.getAll()[i].setBills(unpaidCount);
    }
}

// Searches doctors by specialization, lets the patient pick a slot, and creates an appointment and bill.
static void bookAppointment(
    Patient* pat,
    Storage<Doctor>& doctors,
    Storage<Appointment>& appointments,
    Storage<Bill>& bills,
    Storage<Patient>& patients
) {
    char searchTerm[51] = {};
    cout << "Enter specialization to search (e.g., Cardiology): ";
    cin.ignore(1000, '\n');
    cin.getline(searchTerm, 51);

    for (int i = 0; searchTerm[i] != '\0'; i++)
        searchTerm[i] = toLowerChar(searchTerm[i]);

    bool anyDoctorFound = false;
    for (int i = 0; i < doctors.size(); i++) {
        Doctor& doctor = doctors.getAll()[i];
        if (strEqCI(doctor.getSpecialization(), searchTerm)) {
            anyDoctorFound = true;
            cout << "Doctor ID: " << doctor.getID()
                 << " | Name: "   << doctor.getName()
                 << " | Fee: PKR "<< doctor.getFee() << endl;
        }
    }

    if (!anyDoctorFound) {
        cout << "No doctors available." << endl;
        return;
    }

    int doctorID;
    cout << "Enter Doctor ID: ";
    cin >> doctorID;

    Doctor* doctor = doctors.findByID(doctorID);
    if (doctor == nullptr) {
        cout << "Doctor not found." << endl;
        return;
    }

    char date[11] = {};
    bool validDate = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        cout << "Enter date (DD-MM-YYYY): ";
        cin.ignore(1000, '\n');
        cin.getline(date, 11);
        if (Validator::validateDate(date)) { validDate = true; break; }
        cout << "Invalid date. Use format DD-MM-YYYY." << endl;
    }
    if (!validDate) return;

    cout << "Available time slots:" << endl;
    bool hasAvailableSlot = false;
    for (int i = 0; i < 8; i++) {
        Appointment probe(0, pat->getID(), doctorID, date, SLOTS[i], "pending");
        bool taken = false;
        for (int j = 0; j < appointments.size(); j++) {
            if (probe == appointments.getAll()[j]) { taken = true; break; }
        }
        if (!taken) {
            hasAvailableSlot = true;
            cout << "  " << SLOTS[i] << endl;
        }
    }
    if (!hasAvailableSlot) {
        cout << "No available slots for this doctor on that date." << endl;
        return;
    }

    char slot[6] = {};
    cout << "Enter time slot (e.g., 09:00): ";
    cin >> slot;

    if (!Validator::validateTimeSlot(slot)) {
        cout << "Invalid time slot." << endl;
        return;
    }

    try {
        Appointment newAppt(0, pat->getID(), doctorID, date, slot, "pending");
        for (int i = 0; i < appointments.size(); i++) {
            if (newAppt == appointments.getAll()[i])
                throw SlotUnavailableException(slot);
        }

        if (!(*pat >= doctor->getFee()))
            throw InsufficientFundsException("Insufficient funds. Please top up your balance.");

        *pat -= doctor->getFee();

        char today[11];
        getTodayDate(today);
        int appointmentID = nextAppointmentID(appointments);
        int billID        = nextBillID(bills);

        Appointment appt(appointmentID, pat->getID(), doctorID, date, slot, "pending");
        Bill        bill(billID, pat->getID(), appointmentID, doctor->getFee(), "unpaid", today);

        appointments.add(appt);
        bills.add(bill);

        fileHandler::appendAppointment(appt);
        fileHandler::appendBill(bill);
        fileHandler::saveAllPatients(patients);
        syncBillCounts(patients, bills);

        cout << "Appointment booked successfully. Appointment ID: "
             << appointmentID << "." << endl;

    } catch (SlotUnavailableException& e) {
        cout << e.what() << endl;
    } catch (InsufficientFundsException& e) {
        cout << e.what() << endl;
    }
}

// Lists the patient's pending appointments and cancels the chosen one, refunding the fee.
static void cancelAppointment(
    Patient*              pat,
    Storage<Doctor>&      doctors,
    Storage<Appointment>& appointments,
    Storage<Bill>&        bills,
    Storage<Patient>&     patients
) {
    bool hasPending = false;
    for (int i = 0; i < appointments.size(); i++) {
        Appointment& appt = appointments.getAll()[i];
        if (appt.getPatientID() != pat->getID() || !appt.isPending()) continue;
        hasPending = true;
        Doctor* doctor = doctors.findByID(appt.getDoctorID());
        cout << "Appointment ID: " << appt.getID()
             << " | Doctor: "      << (doctor ? doctor->getName() : "Unknown")
             << " | Date: "        << appt.getDate()
             << " | Time Slot: "   << appt.getTimeSlot()
             << endl;
    }
    if (!hasPending) {
        cout << "You have no pending appointments." << endl;
        return;
    }

    int appointmentID;
    cout << "Enter Appointment ID to cancel: ";
    cin >> appointmentID;

    Appointment* appt = appointments.findByID(appointmentID);
    if (appt == nullptr ||
        appt->getPatientID() != pat->getID() ||
        !appt->isPending()) {
        cout << "Invalid appointment ID." << endl;
        return;
    }

    Doctor* doctor = doctors.findByID(appt->getDoctorID());
    double  fee    = doctor ? doctor->getFee() : 0.0;
    *pat += fee;

    appt->setStatus("cancelled");

    for (int i = 0; i < bills.size(); i++) {
        if (bills.getAll()[i].getAppointmentID() == appointmentID) {
            bills.getAll()[i].setStatus("cancelled");
            break;
        }
    }

    fileHandler::saveAllPatients    (patients);
    fileHandler::saveAllAppointments(appointments);
    fileHandler::saveAllBills       (bills);
    syncBillCounts(patients, bills);

    cout << "Appointment cancelled. PKR " << fee
         << " refunded to your balance." << endl;
}

// Displays the patient's appointments sorted by date ascending.
static void viewMyAppointments(
    Patient*              pat,
    Storage<Doctor>&      doctors,
    Storage<Appointment>& appointments
) {
    int total = appointments.size();
    Appointment** myAppointments = new Appointment*[total];
    int apptCount = 0;

    for (int i = 0; i < total; i++)
        if (appointments.getAll()[i].getPatientID() == pat->getID())
            myAppointments[apptCount++] = &appointments.getAll()[i];

    if (apptCount == 0) {
        cout << "No appointments found." << endl;
        delete[] myAppointments;
        return;
    }

    // Bubble sort ascending by date
    for (int i = 0; i < apptCount - 1; i++)
        for (int j = 0; j < apptCount - i - 1; j++)
            if (dateToInt(myAppointments[j]->getDate()) > dateToInt(myAppointments[j + 1]->getDate())) {
                Appointment* temp = myAppointments[j];
                myAppointments[j] = myAppointments[j + 1];
                myAppointments[j + 1] = temp;
            }

    cout << "=== My Appointments ===" << endl;
    for (int i = 0; i < apptCount; i++) {
        Appointment* appt = myAppointments[i];
        Doctor* doctor = doctors.findByID(appt->getDoctorID());
        cout << "ID: "  << appt->getID()
             << " | Doctor: " << (doctor ? doctor->getName() : "Unknown")
             << " | Specialization: " << (doctor ? doctor->getSpecialization() : "Unknown")
             << " | Date: " << appt->getDate()
             << " | Time Slot: " << appt->getTimeSlot()
             << " | Status: " << appt->getStatus()
             << endl;
    }

    delete[] myAppointments;
}

// Displays the patient's prescriptions sorted by date descending (most recent first).
static void viewMyMedicalRecords(
    Patient* pat,
    Storage<Doctor>& doctors,
    Storage<Prescription>& prescriptions
) {
    int total = prescriptions.size();
    Prescription** myPrescriptions = new Prescription*[total];
    int prescCount = 0;

    for (int i = 0; i < total; i++)
        if (prescriptions.getAll()[i].getPatientID() == pat->getID())
            myPrescriptions[prescCount++] = &prescriptions.getAll()[i];

    if (prescCount == 0) {
        cout << "No medical records found." << endl;
        delete[] myPrescriptions;
        return;
    }

    // Bubble sort descending by date (most recent first)
    for (int i = 0; i < prescCount - 1; i++)
        for (int j = 0; j < prescCount - i - 1; j++)
            if (dateToInt(myPrescriptions[j]->getDate()) < dateToInt(myPrescriptions[j + 1]->getDate())) {
                Prescription* temp = myPrescriptions[j];
                myPrescriptions[j] = myPrescriptions[j + 1];
                myPrescriptions[j + 1] = temp;
            }

    cout << "=== My Medical Records ===" << endl;
    for (int i = 0; i < prescCount; i++) {
        Doctor* doctor     = doctors.findByID(myPrescriptions[i]->getDoctorID());
        const char* doctorName = doctor ? doctor->getName() : "Unknown";
        myPrescriptions[i]->displayForPatient(doctorName);
    }

    delete[] myPrescriptions;
}

// Displays all bills for the patient and the total unpaid amount.
static void viewMyBills(Patient* pat, Storage<Bill>& bills) {

    bool   found       = false;
    double totalUnpaid = 0.0;

    cout << "=== My Bills ===" << endl;

    for (int i = 0; i < bills.size(); i++) {
        Bill& bill = bills.getAll()[i];
        if (bill.getPatientID() != pat->getID()) continue;
        found = true;
        cout << bill << endl;
        if (bill.isUnpaid()) totalUnpaid += bill.getAmount();
    }

    if (!found) { cout << "No bills found." << endl; return; }

    cout << "Total outstanding (unpaid): PKR " << totalUnpaid << endl;
}

// Lists unpaid bills, prompts for a bill ID, and deducts the amount from the patient's balance.
static void payBill(
    Patient*          pat,
    Storage<Bill>&    bills,
    Storage<Patient>& patients
) {
    bool hasUnpaid = false;
    for (int i = 0; i < bills.size(); i++) {
        Bill& bill = bills.getAll()[i];
        if (bill.getPatientID() == pat->getID() && bill.isUnpaid()) {
            hasUnpaid = true;
            cout << bill << endl;
        }
    }
    if (!hasUnpaid) {
        cout << "No unpaid bills." << endl;
        return;
    }

    int billID;
    cout << "Enter Bill ID to pay: ";
    cin >> billID;

    Bill* bill = bills.findByID(billID);
    if (bill == nullptr ||
        bill->getPatientID() != pat->getID() ||
        !bill->isUnpaid()) {
        cout << "Invalid bill ID." << endl;
        return;
    }

    try {
        if (!(*pat >= bill->getAmount()))
            throw InsufficientFundsException("Insufficient funds. Please top up your balance.");

        *pat -= bill->getAmount();
        bill->setStatus("paid");

        fileHandler::saveAllPatients(patients);
        fileHandler::saveAllBills   (bills);
        syncBillCounts(patients, bills);

        cout << "Bill paid successfully. Remaining balance: PKR "
             << pat->getBalance() << "." << endl;

    } catch (InsufficientFundsException& e) {
        cout << e.what() << endl;
    }
}

// Prompts for an amount (up to 3 attempts) and adds it to the patient's balance.
static void topUpBalance(Patient* pat, Storage<Patient>& patients) {

    for (int attempt = 0; attempt < 3; attempt++) {
        double amount = 0.0;
        cout << "Enter amount to add (PKR): ";
        cin >> amount;

        try {
            if (!Validator::validatePositiveFloat(amount))
                throw InvalidInputException("Amount must be a positive number greater than 0.");

            *pat += amount;
            fileHandler::saveAllPatients(patients);

            cout << "Balance updated. New balance: PKR "
                 << pat->getBalance() << "." << endl;
            return;

        } catch (InvalidInputException& e) {
            cout << e.what() << endl;
        }
    }

    cout << "Too many invalid attempts. Returning to menu." << endl;
}

static void patientMenu(
    Patient* pat,
    Storage<Doctor>& doctors,
    Storage<Appointment>& appointments,
    Storage<Bill>& bills,
    Storage<Prescription>& prescriptions,
    Storage<Patient>& patients
) {
    int choice = 0;

    do {
        cout << "\nWelcome, " << pat->getName()    << endl;
        cout << "Balance: PKR " << pat->getBalance() << endl;
        cout << "=======================" << endl;
        cout << "1. Book Appointment" << endl;
        cout << "2. Cancel Appointment" << endl;
        cout << "3. View My Appointments" << endl;
        cout << "4. View My Medical Records" << endl;
        cout << "5. View My Bills" << endl;
        cout << "6. Pay Bill" << endl;
        cout << "7. Top Up Balance" << endl;
        cout << "8. Logout" << endl;
        cout << "Enter choice: ";
        choice = readIntInRange(1, 8);

        switch (choice) {
            case 1: bookAppointment   (pat, doctors, appointments, bills, patients); break;
            case 2: cancelAppointment (pat, doctors, appointments, bills, patients); break;
            case 3: viewMyAppointments(pat, doctors, appointments);                  break;
            case 4: viewMyMedicalRecords(pat, doctors, prescriptions);               break;
            case 5: viewMyBills       (pat, bills);                                  break;
            case 6: payBill           (pat, bills, patients);                        break;
            case 7: topUpBalance      (pat, patients);                               break;
            case 8: cout << "Logged out." << endl;                                   break;
        }

    } while (choice != 8);
}

static void doctorMenu(Doctor* doctor) {

    int choice = 0;

    do {
        cout << "\nWelcome, Dr. " << doctor->getName()
             << "  |  Specialization: " << doctor->getSpecialization() << endl;
        cout << "=============================================" << endl;
        cout << "1. View Today's Appointments" << endl;
        cout << "2. Mark Appointment Complete" << endl;
        cout << "3. Mark Appointment No-Show" << endl;
        cout << "4. Write Prescription"  << endl;
        cout << "5. View Patient Medical History" << endl;
        cout << "6. Logout"  << endl;
        cout << "Enter choice: ";
        choice = readIntInRange(1, 6);

        if (choice == 1) {
            doctor->viewTodaysAppointments();

        } else if (choice == 2) {
            doctor->viewTodaysAppointments();
            int appointmentID;
            cout << "Enter Appointment ID to mark complete: ";
            cin >> appointmentID;
            doctor->markAppointmentComplete(appointmentID);

        } else if (choice == 3) {
            doctor->viewTodaysAppointments();
            int appointmentID;
            cout << "Enter Appointment ID to mark no-show: ";
            cin >> appointmentID;
            doctor->markAppointmentNoShow(appointmentID);

        } else if (choice == 4) {
            doctor->viewTodaysAppointments();

            int appointmentID;
            cout << "Enter Appointment ID: ";
            cin >> appointmentID;

            char medicines[500] = {};
            cout << "Enter medicines (format: MedicineName Dosage;MedicineName Dosage): ";
            cin.ignore(1000, '\n');
            cin.getline(medicines, 500);

            char notes[300] = {};
            cout << "Enter notes (max 300 chars): ";
            cin.getline(notes, 300);

            doctor->writePrescription(appointmentID, medicines, notes);

        } else if (choice == 5) {
            int patientID;
            cout << "Enter Patient ID: ";
            cin >> patientID;
            doctor->viewPatientMedicalHistory(patientID);

        } else if (choice == 6) {
            cout << "Logged out." << endl;
        }

    } while (choice != 6);
}

static void adminMenu(Admin* admin) {

    int choice = 0;

    do {
        cout << "\nAdmin Panel - MediCore" << endl;
        cout << "======================" << endl;
        cout << " 1. Add Doctor"            << endl;
        cout << " 2. Remove Doctor"         << endl;
        cout << " 3. Add Patient"           << endl;
        cout << " 4. Remove Patient"        << endl;
        cout << " 5. View All Patients"     << endl;
        cout << " 6. View All Doctors"      << endl;
        cout << " 7. View All Appointments" << endl;
        cout << " 8. View Unpaid Bills"     << endl;
        cout << " 9. Discharge Patient"     << endl;
        cout << "10. View Security Log"     << endl;
        cout << "11. Generate Daily Report" << endl;
        cout << "12. Logout"                << endl;
        cout << "Enter choice: ";
        choice = readIntInRange(1, 12);

        switch (choice) {
            case  1: admin->addDoctor();           break;
            case  2: admin->removeDoctor();        break;
            case  3: admin->addPatient();          break;
            case  4: admin->removePatient();       break;
            case  5: admin->viewAllPatients();     break;
            case  6: admin->viewAllDoctors();      break;
            case  7: admin->viewAllAppointments(); break;
            case  8: admin->viewUnpaidBills();     break;
            case  9: admin->dischargePatient();    break;
            case 10: admin->viewSecurityLog();     break;
            case 11: admin->generateDailyReport(); break;
            case 12: cout << "Logged out." << endl; break;
        }

    } while (choice != 12);
}

static void loginPatient(
    Storage<Patient>& patients,
    Storage<Doctor>& doctors,
    Storage<Appointment>& appointments,
    Storage<Bill>& bills,
    Storage<Prescription>& prescriptions
) {
    int  patientID;
    char password[31] = {};

    cout << "Enter Patient ID: ";
    cin  >> patientID;

    Patient* patient = patients.findByID(patientID);
    if (patient == nullptr) {
        logSecurity("Patient", patientID, false);
        cout << "Patient not found." << endl;
        return;
    }

    cout << "Enter password: ";
    cin  >> password;

    if (!strEq(password, patient->getPassword())) {
        logSecurity("Patient", patientID, false);
        cout << "Incorrect password." << endl;
        return;
    }

    logSecurity("Patient", patientID, true);
    patientMenu(patient, doctors, appointments, bills, prescriptions, patients);
}

static void loginDoctor(
    Storage<Doctor>&       doctors,
    Storage<Patient>&      patients,
    Storage<Appointment>&  appointments,
    Storage<Bill>&         bills,
    Storage<Prescription>& prescriptions
) {
    int  doctorID;
    char password[31] = {};

    cout << "Enter Doctor ID: ";
    cin  >> doctorID;

    Doctor* doctor = doctors.findByID(doctorID);
    if (doctor == nullptr) {
        logSecurity("Doctor", doctorID, false);
        cout << "Doctor not found." << endl;
        return;
    }

    cout << "Enter password: ";
    cin  >> password;

    if (!strEq(password, doctor->getPassword())) {
        logSecurity("Doctor", doctorID, false);
        cout << "Incorrect password." << endl;
        return;
    }

    logSecurity("Doctor", doctorID, true);
    doctor->setStorages(&appointments, &patients, &prescriptions, &bills);
    doctorMenu(doctor);
}

static void loginAdmin(
    Storage<Admin>&        admins,
    Storage<Patient>&      patients,
    Storage<Doctor>&       doctors,
    Storage<Appointment>&  appointments,
    Storage<Bill>&         bills,
    Storage<Prescription>& prescriptions
) {
    int  adminID;
    char password[31] = {};

    cout << "Enter Admin ID: ";
    cin  >> adminID;

    Admin* admin = admins.findByID(adminID);
    if (admin == nullptr) {
        logSecurity("Admin", adminID, false);
        cout << "Admin not found." << endl;
        return;
    }

    cout << "Enter password: ";
    cin  >> password;

    if (!strEq(password, admin->getPassword())) {
        logSecurity("Admin", adminID, false);
        cout << "Incorrect password." << endl;
        return;
    }

    logSecurity("Admin", adminID, true);
    admin->setStorages(&patients, &doctors, &appointments, &bills, &prescriptions);
    adminMenu(admin);
}

int main() {

    Storage<Patient>      patients;
    Storage<Doctor>       doctors;
    Storage<Admin>        admins;
    Storage<Appointment>  appointments;
    Storage<Bill>         bills;
    Storage<Prescription> prescriptions;

    try {
        fileHandler::loadPatients     (patients);
        fileHandler::loadDoctors      (doctors);
        fileHandler::loadAppointments (appointments);
        fileHandler::loadBills        (bills);
        fileHandler::loadPrescriptions(prescriptions);
    } catch (FileNotFoundException& e) {
        cout << e.what() << endl;
        return 1;
    }

    // Load admins from admin.txt (format: admin_id,name,password)
    try {
        ifstream adminFile("admin.txt");
        if (!adminFile.is_open()) throw FileNotFoundException("admin.txt");
        
        char line[200];
        
        adminFile.getline(line, 200);   // skip header
        while (adminFile.getline(line, 200)) {

            char* token;
            token = strtok(line, ",");

            int adminId = atoi(token);

            token = strtok(NULL, ",");

            char* adminName = new char[lenArr(token) + 1];

            for (int i = 0; i <= lenArr(token); i++) adminName[i] = token[i];

            token = strtok(NULL, ",");

            char* adminPassword = new char[lenArr(token) + 1];
            for (int i = 0; i <= lenArr(token); i++) adminPassword[i] = token[i];

            Admin admin(adminId, adminName, adminPassword);
            admins.add(admin);
            
            delete[] adminName;
            delete[] adminPassword;
        }
        adminFile.close();
    } catch (FileNotFoundException& e) {
        cout << e.what() << endl;
        return 1;
    }

    syncBillCounts(patients, bills);

    cout << "Welcome to MediCore Hospital Management System" << endl;
    cout << "===============================================" << endl;

    int choice = 0;

    do {
        cout << endl << "Login as:" << endl;
        cout << "1. Patient" << endl;
        cout << "2. Doctor" << endl;
        cout << "3. Admin" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter choice: ";
        choice = readIntInRange(1, 4);

        switch (choice) {
            case 1:
                loginPatient(patients, doctors, appointments, bills, prescriptions);
                break;
            case 2:
                loginDoctor(doctors, patients, appointments, bills, prescriptions);
                break;
            case 3:
                loginAdmin(admins, patients, doctors, appointments, bills, prescriptions);
                break;
            case 4:
                cout << "Goodbye." << endl;
                break;
        }

    } while (choice != 4);
}
