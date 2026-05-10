#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "Header/constants.h"
#include "Header/Storage.h"
#include "Header/Patient.h"
#include "Header/Doctor.h"
#include "Header/admin.h"
#include "Header/Appointment.h"
#include "Header/Bill.h"
#include "Header/Prescription.h"
#include "Header/FileHandler.h"
#include "Header/Validator.h"
#include "Header/Exceptions.h"

static const unsigned WIN_W = 900;
static const unsigned WIN_H = 620;

static const sf::Color C_BG     {10,  15,  26};
static const sf::Color C_SURF   {22,  30,  50};
static const sf::Color C_SURF2  {35,  48,  78};
static const sf::Color C_BLUE   {37, 112, 255};
static const sf::Color C_TEAL   {20, 184, 166};
static const sf::Color C_GREEN  {34, 197,  94};
static const sf::Color C_RED    {220,  60,  60};
static const sf::Color C_AMBER  {220, 170,   0};
static const sf::Color C_WHITE  {230, 238, 255};
static const sf::Color C_GREY   {140, 155, 175};
static const sf::Color C_DARK   { 65,  80, 105};
static const sf::Color C_BORDER { 42,  58,  90};
static const sf::Color C_INP    {  8,  13,  28};

static sf::Font gFont;

static Storage<Patient>      gPat;
static Storage<Doctor>       gDoc;
static Storage<Admin>        gAdm;
static Storage<Appointment>  gAppt;
static Storage<Bill>         gBill;
static Storage<Prescription> gPresc;

enum class Screen  { LAUNCHER, LOGIN, ROLE, TERMINAL };

enum class Feature {
    NONE,
    P_BOOK_SPEC, P_BOOK_DOCID, P_BOOK_DATE, P_BOOK_SLOT,
    P_CANCEL, P_VIEW_APPTS, P_VIEW_RECORDS,
    P_VIEW_BILLS, P_PAY_BILL, P_TOPUP,
    D_VIEW_TODAY, D_MARK_COMPLETE, D_MARK_NOSHOW,
    D_WRITE_RX_ID, D_WRITE_RX_MED, D_WRITE_RX_NOTE,
    D_VIEW_HISTORY,
    A_ADD_DOC, A_REM_DOC, A_ADD_PAT, A_REM_PAT,
    A_VIEW_PATS, A_VIEW_DOCS, A_VIEW_APPTS,
    A_VIEW_UNPAID, A_DISCHARGE, A_SEC_LOG, A_DAILY_REPORT
};

static bool strEq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == '\0' && *b == '\0';
}

static char toLowerChar(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static bool strEqCI(const char* a, const char* b) {
    while (*a && *b) {
        if (toLowerChar(*a) != toLowerChar(*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

// Converts "DD-MM-YYYY" to integer YYYYMMDD for chronological comparison.
static int dateToInt(const char* date) {
    int day   = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year  = (date[6] - '0') * 1000 + (date[7] - '0') * 100
              + (date[8] - '0') * 10   + (date[9] - '0');
    return year * 10000 + month * 100 + day;
}

// Writes today's date into buf as "DD-MM-YYYY". buf must hold at least 11 bytes.
static void getTodayDate(char* buf) {
    time_t now = time(0);
    tm* localTime = localtime(&now);
    strftime(buf, 11, "%d-%m-%Y", localTime);
}

static int nextAppointmentID() {
    int maxID = 0;
    for (int i = 0; i < gAppt.size(); i++)
        if (gAppt.getAll()[i].getID() > maxID) maxID = gAppt.getAll()[i].getID();
    return maxID + 1;
}

static int nextBillID() {
    int maxID = 0;
    for (int i = 0; i < gBill.size(); i++)
        if (gBill.getAll()[i].getID() > maxID) maxID = gBill.getAll()[i].getID();
    return maxID + 1;
}

static void syncBillCounts() {
    for (int i = 0; i < gPat.size(); i++) {
        int patientID = gPat.getAll()[i].getID();
        int unpaidCount = 0;
        for (int j = 0; j < gBill.size(); j++)
            if (gBill.getAll()[j].getPatientID() == patientID && gBill.getAll()[j].isUnpaid())
                unpaidCount++;
        gPat.getAll()[i].setBills(unpaidCount);
    }
}

static string trimStr(const string& str) {
    size_t start = 0;
    while (start < str.size() && isspace((unsigned char)str[start])) start++;
    size_t end = str.size();
    while (end > start && isspace((unsigned char)str[end - 1])) end--;
    return str.substr(start, end - start);
}

// Captures everything printed to cout by fn() and returns it as a string.
template<typename F>
static string captureOutput(F fn) {
    ostringstream outputStream;
    streambuf* originalBuffer = cout.rdbuf(outputStream.rdbuf());
    fn();
    cout.rdbuf(originalBuffer);
    return outputStream.str();
}

static const char* SLOTS[8] = {
    "09:00", "10:00", "11:00", "12:00",
    "13:00", "14:00", "15:00", "16:00"
};

static bool containsPoint(const sf::FloatRect& rect, float pointX, float pointY) {
    return rect.contains(pointX, pointY);
}

static void fillRect(sf::RenderWindow& window, float x, float y, float rectW, float rectH,
                     sf::Color fillColor, sf::Color outlineColor = sf::Color::Transparent,
                     float outlineThickness = 0.f) {
    sf::RectangleShape rect({rectW, rectH});
    rect.setPosition({x, y});
    rect.setFillColor(fillColor);
    if (outlineThickness > 0.f) {
        rect.setOutlineColor(outlineColor);
        rect.setOutlineThickness(outlineThickness);
    }
    window.draw(rect);
}

static void drawHLine(sf::RenderWindow& window, float x, float y, float lineWidth,
                      sf::Color color = C_BORDER) {
    fillRect(window, x, y, lineWidth, 1.f, color);
}

static void drawText(sf::RenderWindow& window, const string& str,
                     float x, float y, unsigned fontSize, sf::Color color = C_WHITE,
                     bool centreX = false) {
    sf::Text text(str, gFont, fontSize);
    text.setFillColor(color);
    if (centreX) {
        auto textBounds = text.getLocalBounds();
        x -= textBounds.left + textBounds.width / 2.f;
    }
    text.setPosition({x, y});
    window.draw(text);
}

static void drawButton(sf::RenderWindow& window, const sf::FloatRect& rect,
                       const string& label, sf::Color fillColor, bool isHovered,
                       unsigned fontSize = 15) {
    sf::Color buttonColor = isHovered ? sf::Color(
        (sf::Uint8)std::min(255, (int)fillColor.r + 45),
        (sf::Uint8)std::min(255, (int)fillColor.g + 45),
        (sf::Uint8)std::min(255, (int)fillColor.b + 45)) : fillColor;
    fillRect(window, rect.left, rect.top, rect.width, rect.height, buttonColor);
    sf::Text text(label, gFont, fontSize);
    text.setFillColor(C_WHITE);
    auto textBounds = text.getLocalBounds();
    text.setPosition({
        rect.left + rect.width  / 2.f - textBounds.left - textBounds.width  / 2.f,
        rect.top  + rect.height / 2.f - textBounds.top  - textBounds.height / 2.f
    });
    window.draw(text);
}

static void drawInputBox(sf::RenderWindow& window, const sf::FloatRect& rect,
                         const string& value, bool isFocused, bool isPassword = false) {
    fillRect(window, rect.left, rect.top, rect.width, rect.height, C_INP,
             isFocused ? C_BLUE : C_BORDER, 1.5f);
    string displayText = isPassword ? string(value.size(), '*') : value;
    if (isFocused) displayText += '|';
    sf::Text text(displayText, gFont, 15);
    text.setFillColor(C_WHITE);
    text.setPosition({rect.left + 10.f, rect.top + rect.height / 2.f - 9.f});
    window.draw(text);
}

struct Terminal {
    static const int CAP = 200;
    char lines[CAP][300];
    int  lineCount    = 0;
    int  scrollOffset = 0;

    void clear() {
        lineCount = 0;
        scrollOffset = 0;
        memset(lines, 0, sizeof(lines));
    }

    void push(const char* str) {
        if (lineCount >= CAP) {
            memmove(lines[0], lines[1], (CAP - 1) * 300);
            lineCount = CAP - 1;
        }
        strncpy(lines[lineCount++], str, 299);
        int visibleLines = 22;
        scrollOffset = (lineCount > visibleLines) ? lineCount - visibleLines : 0;
    }

    void pushStr(const string& str) {
        int lineStart = 0;
        for (int i = 0; i <= (int)str.size(); i++) {
            if (str[i] == '\n' || str[i] == '\0') {
                char buf[300] = {};
                int lineLen = i - lineStart;
                if (lineLen > 299) lineLen = 299;
                strncpy(buf, str.c_str() + lineStart, lineLen);
                if (buf[0] || lineLen == 0) push(buf);
                lineStart = i + 1;
            }
        }
    }

    void scrollUp(int delta = 3) {
        scrollOffset = (scrollOffset > delta) ? scrollOffset - delta : 0;
    }

    void scrollDown(int delta = 3) {
        int visibleLines = 22;
        scrollOffset = std::min(std::max(0, lineCount - visibleLines), scrollOffset + delta);
    }
};

struct AppState {
    Screen  screen  = Screen::LAUNCHER;
    Feature feature = Feature::NONE;
    int     role    = 0;   // 1=patient 2=doctor 3=admin

    Patient* pat = nullptr;
    Doctor*  doc = nullptr;
    Admin*   adm = nullptr;

    string loginId;
    string loginPassword;
    bool   idFieldFocused = true;
    string loginError;

    Terminal terminal;
    string   inputText;
    bool     inputIsPassword = false;
    string   prompt;

    string wizardValues[8];
    int    wizardStep       = 0;
    int    selectedDoctorId = 0;
};

static void logSecurityEvent(const char* role, int id, bool success) {
    ofstream logFile("security_log.txt", ios::app);
    char timestamp[20] = {};
    time_t now = time(0);
    strftime(timestamp, 20, "%d-%m-%Y %H:%M:%S", localtime(&now));
    logFile << timestamp << "," << role << "," << id << "," << (success ? "SUCCESS" : "FAILED") << "\n";
}

static void attemptLogin(AppState& state) {
    int enteredId    = atoi(state.loginId.c_str());
    string enteredPw = trimStr(state.loginPassword);

    if (state.role == 1) {
        Patient* patient = gPat.findByID(enteredId);
        if (!patient || trimStr(string(patient->getPassword())) != enteredPw) {
            logSecurityEvent("Patient", enteredId, false);
            state.loginError = "Invalid ID or password.";
            return;
        }
        logSecurityEvent("Patient", enteredId, true);
        state.pat    = patient;
        state.screen = Screen::ROLE;
        state.loginError = "";

    } else if (state.role == 2) {
        Doctor* doctor = gDoc.findByID(enteredId);
        if (!doctor || trimStr(string(doctor->getPassword())) != enteredPw) {
            logSecurityEvent("Doctor", enteredId, false);
            state.loginError = "Invalid ID or password.";
            return;
        }
        logSecurityEvent("Doctor", enteredId, true);
        doctor->setStorages(&gAppt, &gPat, &gPresc, &gBill);
        state.doc    = doctor;
        state.screen = Screen::ROLE;
        state.loginError = "";

    } else {
        Admin* admin = gAdm.findByID(enteredId);
        if (!admin || trimStr(string(admin->getPassword())) != enteredPw) {
            logSecurityEvent("Admin", enteredId, false);
            state.loginError = "Invalid ID or password.";
            return;
        }
        logSecurityEvent("Admin", enteredId, true);
        admin->setStorages(&gPat, &gDoc, &gAppt, &gBill, &gPresc);
        state.adm    = admin;
        state.screen = Screen::ROLE;
        state.loginError = "";
    }
}

static void launchFeature(AppState& state, Feature feature);

static void setTerminalPrompt(AppState& state, const char* promptText, bool isPassword = false) {
    state.prompt         = promptText;
    state.inputText.clear();
    state.inputIsPassword = isPassword;
}

static void onSubmit(AppState& state) {
    if (state.prompt.empty()) return;
    string echo = "> " + (state.inputIsPassword ? "****" : state.inputText);
    state.terminal.push(echo.c_str());

    char messageBuf[520] = {};

    switch (state.feature) {

    case Feature::P_BOOK_SPEC: {
        string specSearch = state.inputText;
        for (auto& c : specSearch) c = toLowerChar(c);
        bool anyDoctors = false;
        for (int i = 0; i < gDoc.size(); i++) {
            Doctor& doctor = gDoc.getAll()[i];
            if (!strEqCI(doctor.getSpecialization(), specSearch.c_str())) continue;
            anyDoctors = true;
            snprintf(messageBuf, 519, "  ID:%-3d  %-22s  Fee: PKR %.0f",
                     doctor.getID(), doctor.getName(), doctor.getFee());
            state.terminal.push(messageBuf);
        }
        if (!anyDoctors) {
            state.terminal.push("! No doctors for that specialization.");
            state.prompt = "";
            break;
        }
        state.wizardValues[state.wizardStep++] = state.inputText;
        state.feature = Feature::P_BOOK_DOCID;
        setTerminalPrompt(state, "Doctor ID:");
        break;
    }

    case Feature::P_BOOK_DOCID: {
        int doctorID = atoi(state.inputText.c_str());
        Doctor* doctor = gDoc.findByID(doctorID);
        if (!doctor) { state.terminal.push("! Doctor not found."); break; }
        state.selectedDoctorId = doctorID;
        state.wizardValues[state.wizardStep++] = state.inputText;
        state.feature = Feature::P_BOOK_DATE;
        setTerminalPrompt(state, "Appointment date (DD-MM-YYYY):");
        break;
    }

    case Feature::P_BOOK_DATE: {
        if (!Validator::validateDate(state.inputText.c_str())) {
            state.terminal.push("! Invalid date. Use DD-MM-YYYY.");
            break;
        }
        state.wizardValues[state.wizardStep++] = state.inputText;
        char date[11];
        strncpy(date, state.inputText.c_str(), 10);
        date[10] = 0;
        char freeSlotLine[300] = "  Free: ";
        bool hasFreeSlot = false;
        for (int i = 0; i < 8; i++) {
            Appointment probe(0, state.pat->getID(), state.selectedDoctorId, date, SLOTS[i], "pending");
            bool taken = false;
            for (int j = 0; j < gAppt.size(); j++)
                if (probe == gAppt.getAll()[j]) { taken = true; break; }
            if (!taken) {
                strncat(freeSlotLine, SLOTS[i], 6);
                strncat(freeSlotLine, "  ", 3);
                hasFreeSlot = true;
            }
        }
        if (!hasFreeSlot) {
            state.terminal.push("! No free slots on that date.");
            state.prompt = "";
            break;
        }
        state.terminal.push(freeSlotLine);
        state.feature = Feature::P_BOOK_SLOT;
        setTerminalPrompt(state, "Time slot (e.g. 09:00):");
        break;
    }

    case Feature::P_BOOK_SLOT: {
        if (!Validator::validateTimeSlot(state.inputText.c_str())) {
            state.terminal.push("! Invalid slot.");
            break;
        }
        char slot[6];  strncpy(slot, state.inputText.c_str(), 5);  slot[5]  = 0;
        char date[11]; strncpy(date, state.wizardValues[2].c_str(), 10); date[10] = 0;
        int doctorID = state.selectedDoctorId;
        Appointment probe(0, state.pat->getID(), doctorID, date, slot, "pending");
        bool slotConflict = false;
        for (int i = 0; i < gAppt.size(); i++)
            if (probe == gAppt.getAll()[i]) { slotConflict = true; break; }
        if (slotConflict) { state.terminal.push("! Slot already taken."); break; }
        Doctor* doctor = gDoc.findByID(doctorID);
        try {
            if (!(*state.pat >= doctor->getFee()))
                throw InsufficientFundsException("Insufficient funds.");
            *state.pat -= doctor->getFee();
            char today[11]; getTodayDate(today);
            int appointmentID = nextAppointmentID();
            int billID        = nextBillID();
            Appointment appt(appointmentID, state.pat->getID(), doctorID, date, slot, "pending");
            Bill        bill(billID, state.pat->getID(), appointmentID, doctor->getFee(), "unpaid", today);
            gAppt.add(appt);
            gBill.add(bill);
            fileHandler::appendAppointment(appt);
            fileHandler::appendBill(bill);
            fileHandler::saveAllPatients(gPat);
            syncBillCounts();
            snprintf(messageBuf, 519, "* Booked! Appointment ID: %d", appointmentID);
            state.terminal.push(messageBuf);
        } catch (InsufficientFundsException& e) {
            snprintf(messageBuf, 519, "! %s", e.what());
            state.terminal.push(messageBuf);
        }
        state.prompt = "";
        break;
    }

    case Feature::P_CANCEL: {
        int appointmentID = atoi(state.inputText.c_str());
        Appointment* appt = gAppt.findByID(appointmentID);
        if (!appt || appt->getPatientID() != state.pat->getID() || !appt->isPending()) {
            state.terminal.push("! Invalid appointment ID.");
            break;
        }
        Doctor* doctor = gDoc.findByID(appt->getDoctorID());
        double  fee    = doctor ? doctor->getFee() : 0.0;
        *state.pat += fee;
        appt->setStatus("cancelled");
        for (int i = 0; i < gBill.size(); i++)
            if (gBill.getAll()[i].getAppointmentID() == appointmentID) {
                gBill.getAll()[i].setStatus("cancelled");
                break;
            }
        fileHandler::saveAllPatients    (gPat);
        fileHandler::saveAllAppointments(gAppt);
        fileHandler::saveAllBills       (gBill);
        syncBillCounts();
        snprintf(messageBuf, 519, "* Cancelled. PKR %.2f refunded.", fee);
        state.terminal.push(messageBuf);
        state.prompt = "";
        break;
    }

    case Feature::P_PAY_BILL: {
        int billID = atoi(state.inputText.c_str());
        Bill* bill = gBill.findByID(billID);
        if (!bill || bill->getPatientID() != state.pat->getID() || !bill->isUnpaid()) {
            state.terminal.push("! Invalid bill ID.");
            break;
        }
        try {
            if (!(*state.pat >= bill->getAmount()))
                throw InsufficientFundsException("Insufficient funds.");
            *state.pat -= bill->getAmount();
            bill->setStatus("paid");
            fileHandler::saveAllPatients(gPat);
            fileHandler::saveAllBills   (gBill);
            syncBillCounts();
            snprintf(messageBuf, 519, "* Paid. Balance: PKR %.2f", state.pat->getBalance());
            state.terminal.push(messageBuf);
        } catch (InsufficientFundsException& e) {
            snprintf(messageBuf, 519, "! %s", e.what());
            state.terminal.push(messageBuf);
        }
        state.prompt = "";
        break;
    }

    case Feature::P_TOPUP: {
        double amount = atof(state.inputText.c_str());
        try {
            if (!Validator::validatePositiveFloat(amount))
                throw InvalidInputException("Must be >0.");
            *state.pat += amount;
            fileHandler::saveAllPatients(gPat);
            snprintf(messageBuf, 519, "* Balance: PKR %.2f", state.pat->getBalance());
            state.terminal.push(messageBuf);
            state.prompt = "";
        } catch (InvalidInputException& e) {
            snprintf(messageBuf, 519, "! %s", e.what());
            state.terminal.push(messageBuf);
        }
        break;
    }

    case Feature::D_MARK_COMPLETE: {
        int appointmentID = atoi(state.inputText.c_str());
        bool success = state.doc->markAppointmentComplete(appointmentID);
        state.terminal.push(success ? "* Marked complete." : "! Invalid appointment ID.");
        state.prompt = "";
        break;
    }

    case Feature::D_MARK_NOSHOW: {
        int appointmentID = atoi(state.inputText.c_str());
        bool success = state.doc->markAppointmentNoShow(appointmentID);
        state.terminal.push(success ? "* Marked no-show." : "! Invalid appointment ID.");
        state.prompt = "";
        break;
    }

    case Feature::D_WRITE_RX_ID:
        state.wizardValues[state.wizardStep++] = state.inputText;
        state.feature = Feature::D_WRITE_RX_MED;
        setTerminalPrompt(state, "Medicines (Name Dose;Name Dose):");
        break;

    case Feature::D_WRITE_RX_MED:
        state.wizardValues[state.wizardStep++] = state.inputText;
        state.feature = Feature::D_WRITE_RX_NOTE;
        setTerminalPrompt(state, "Notes:");
        break;

    case Feature::D_WRITE_RX_NOTE: {
        state.wizardValues[state.wizardStep++] = state.inputText;
        int appointmentID = atoi(state.wizardValues[0].c_str());
        char medicinesBuf[500] = {};
        char notesBuf[300]     = {};
        strncpy(medicinesBuf, state.wizardValues[1].c_str(), 499);
        strncpy(notesBuf,     state.wizardValues[2].c_str(), 299);
        bool success = state.doc->writePrescription(appointmentID, medicinesBuf, notesBuf);
        state.terminal.push(success ? "* Prescription saved." : "! Invalid appointment or duplicate.");
        state.prompt = "";
        break;
    }

    case Feature::D_VIEW_HISTORY: {
        int patientID = atoi(state.inputText.c_str());
        string output = captureOutput([&]() { state.doc->viewPatientMedicalHistory(patientID); });
        state.terminal.pushStr(output);
        state.prompt = "";
        break;
    }

    case Feature::A_ADD_DOC: {
        state.wizardValues[state.wizardStep++] = state.inputText;
        if (state.wizardStep < 5) {
            const char* prompts[] = {
                "Specialization:", "Contact (11 digits):", "Password:", "Fee (PKR):"
            };
            setTerminalPrompt(state, prompts[state.wizardStep - 1], state.wizardStep == 3);
        } else {
            string cinStr = "\n" + state.wizardValues[0] + "\n" + state.wizardValues[1] + "\n"
                          + state.wizardValues[2] + "\n" + state.wizardValues[3] + "\n"
                          + state.wizardValues[4] + "\n";
            ostringstream capturedOutput;
            istringstream inputStream(cinStr);
            streambuf* originalCinBuffer  = cin.rdbuf(inputStream.rdbuf());
            streambuf* originalCoutBuffer = cout.rdbuf(capturedOutput.rdbuf());
            state.adm->addDoctor();
            cin.rdbuf(originalCinBuffer);
            cout.rdbuf(originalCoutBuffer);
            state.terminal.pushStr(capturedOutput.str());
            state.prompt = "";
        }
        break;
    }

    case Feature::A_REM_DOC: {
        int doctorID = atoi(state.inputText.c_str());
        Doctor* doctor = gDoc.findByID(doctorID);
        if (!doctor) { state.terminal.push("! Doctor not found."); break; }
        bool hasPendingAppointments = false;
        for (int i = 0; i < gAppt.size(); i++)
            if (gAppt.getAll()[i].getDoctorID() == doctorID && gAppt.getAll()[i].isPending()) {
                hasPendingAppointments = true;
                break;
            }
        if (hasPendingAppointments) {
            state.terminal.push("! Doctor has pending appointments.");
            break;
        }
        gDoc.removeByID(doctorID);
        fileHandler::saveAllDoctors(gDoc);
        state.terminal.push("* Doctor removed.");
        state.prompt = "";
        break;
    }

    case Feature::A_ADD_PAT: {
        state.wizardValues[state.wizardStep++] = state.inputText;
        if (state.wizardStep < 6) {
            const char* prompts[] = {
                "Age (1-120):", "Gender (M/F):", "Contact (11 digits):", "Password:", "Initial balance:"
            };
            setTerminalPrompt(state, prompts[state.wizardStep - 1], state.wizardStep == 4);
        } else {
            string cinStr = "\n" + state.wizardValues[0] + "\n" + state.wizardValues[1] + "\n"
                          + state.wizardValues[2] + "\n" + state.wizardValues[3] + "\n"
                          + state.wizardValues[4] + "\n" + state.wizardValues[5] + "\n";
            ostringstream capturedOutput;
            istringstream inputStream(cinStr);
            streambuf* originalCinBuffer  = cin.rdbuf(inputStream.rdbuf());
            streambuf* originalCoutBuffer = cout.rdbuf(capturedOutput.rdbuf());
            state.adm->addPatient();
            cin.rdbuf(originalCinBuffer);
            cout.rdbuf(originalCoutBuffer);
            state.terminal.pushStr(capturedOutput.str());
            state.prompt = "";
        }
        break;
    }

    case Feature::A_REM_PAT: {
        int patientID = atoi(state.inputText.c_str());
        Patient* patient = gPat.findByID(patientID);
        if (!patient) { state.terminal.push("! Patient not found."); break; }
        bool hasPendingAppointments = false;
        for (int i = 0; i < gAppt.size(); i++)
            if (gAppt.getAll()[i].getPatientID() == patientID && gAppt.getAll()[i].isPending()) {
                hasPendingAppointments = true;
                break;
            }
        if (hasPendingAppointments) {
            state.terminal.push("! Patient has pending appointments.");
            break;
        }
        bool hasUnpaidBills = false;
        for (int i = 0; i < gBill.size(); i++)
            if (gBill.getAll()[i].getPatientID() == patientID && gBill.getAll()[i].isUnpaid()) {
                hasUnpaidBills = true;
                break;
            }
        if (hasUnpaidBills) {
            state.terminal.push("! Patient has unpaid bills.");
            break;
        }
        fileHandler::deletePatientByID(patientID, gPat, gAppt, gBill, gPresc);
        fileHandler::saveAllPatients     (gPat);
        fileHandler::saveAllAppointments (gAppt);
        fileHandler::saveAllBills        (gBill);
        fileHandler::saveAllPrescriptions(gPresc);
        state.terminal.push("* Patient removed.");
        state.prompt = "";
        break;
    }

    case Feature::A_DISCHARGE: {
        int patientID = atoi(state.inputText.c_str());
        string output = captureOutput([&]() {
            char idBuffer[32];
            snprintf(idBuffer, 32, "%d\n", patientID);
            istringstream inputStream(idBuffer);
            streambuf* originalCinBuffer = cin.rdbuf(inputStream.rdbuf());
            state.adm->dischargePatient();
            cin.rdbuf(originalCinBuffer);
        });
        state.terminal.pushStr(output);
        syncBillCounts();
        state.prompt = "";
        break;
    }

    default: break;
    }

    state.inputText.clear();
}

static void launchFeature(AppState& state, Feature feature) {
    state.feature = feature;
    state.screen  = Screen::TERMINAL;
    state.wizardStep       = 0;
    state.selectedDoctorId = 0;
    state.terminal.clear();
    state.prompt = "";
    state.inputText.clear();

    char lineBuf[520] = {};

    switch (feature) {

    case Feature::P_BOOK_SPEC:
        state.terminal.push("# Book Appointment");
        setTerminalPrompt(state, "Specialization (e.g. Cardiology):");
        break;

    case Feature::P_CANCEL: {
        state.terminal.push("# Cancel Appointment");
        bool hasPending = false;
        for (int i = 0; i < gAppt.size(); i++) {
            Appointment& appt = gAppt.getAll()[i];
            if (appt.getPatientID() != state.pat->getID() || !appt.isPending()) continue;
            hasPending = true;
            Doctor* doctor = gDoc.findByID(appt.getDoctorID());
            snprintf(lineBuf, 519, "  ID:%-3d  Dr.%-16s  %-10s  %s",
                     appt.getID(), doctor ? doctor->getName() : "?",
                     appt.getDate(), appt.getTimeSlot());
            state.terminal.push(lineBuf);
        }
        if (!hasPending) { state.terminal.push("  No pending appointments."); break; }
        setTerminalPrompt(state, "Appointment ID to cancel:");
        break;
    }

    case Feature::P_VIEW_APPTS: {
        state.terminal.push("# My Appointments  (date ascending)");
        int total = gAppt.size();
        Appointment** sortedAppointments = new Appointment*[total];
        int apptCount = 0;
        for (int i = 0; i < total; i++)
            if (gAppt.getAll()[i].getPatientID() == state.pat->getID())
                sortedAppointments[apptCount++] = &gAppt.getAll()[i];
        for (int i = 0; i < apptCount - 1; i++)
            for (int j = 0; j < apptCount - i - 1; j++)
                if (dateToInt(sortedAppointments[j]->getDate()) > dateToInt(sortedAppointments[j + 1]->getDate())) {
                    Appointment* temp            = sortedAppointments[j];
                    sortedAppointments[j]        = sortedAppointments[j + 1];
                    sortedAppointments[j + 1]    = temp;
                }
        if (apptCount == 0) {
            state.terminal.push("  No appointments.");
            delete[] sortedAppointments;
            break;
        }
        for (int i = 0; i < apptCount; i++) {
            Doctor* doctor = gDoc.findByID(sortedAppointments[i]->getDoctorID());
            snprintf(lineBuf, 519, "  ID:%-3d  Dr.%-16s  %-10s  %-5s  %s",
                     sortedAppointments[i]->getID(), doctor ? doctor->getName() : "?",
                     sortedAppointments[i]->getDate(),
                     sortedAppointments[i]->getTimeSlot(),
                     sortedAppointments[i]->getStatus());
            state.terminal.push(lineBuf);
        }
        delete[] sortedAppointments;
        break;
    }

    case Feature::P_VIEW_RECORDS: {
        state.terminal.push("# Medical Records  (newest first)");
        int total = gPresc.size();
        Prescription** sortedPrescriptions = new Prescription*[total];
        int prescCount = 0;
        for (int i = 0; i < total; i++)
            if (gPresc.getAll()[i].getPatientID() == state.pat->getID())
                sortedPrescriptions[prescCount++] = &gPresc.getAll()[i];
        for (int i = 0; i < prescCount - 1; i++)
            for (int j = 0; j < prescCount - i - 1; j++)
                if (dateToInt(sortedPrescriptions[j]->getDate()) < dateToInt(sortedPrescriptions[j + 1]->getDate())) {
                    Prescription* temp             = sortedPrescriptions[j];
                    sortedPrescriptions[j]         = sortedPrescriptions[j + 1];
                    sortedPrescriptions[j + 1]     = temp;
                }
        if (prescCount == 0) {
            state.terminal.push("  No records.");
            delete[] sortedPrescriptions;
            break;
        }
        for (int i = 0; i < prescCount; i++) {
            Doctor* doctor = gDoc.findByID(sortedPrescriptions[i]->getDoctorID());
            snprintf(lineBuf, 519, "  %s  Dr.%s",
                     sortedPrescriptions[i]->getDate(), doctor ? doctor->getName() : "?");
            state.terminal.push(lineBuf);
            snprintf(lineBuf, 519, "    Meds: %s", sortedPrescriptions[i]->getMedicines());
            state.terminal.push(lineBuf);
            snprintf(lineBuf, 519, "    Note: %s", sortedPrescriptions[i]->getNotes());
            state.terminal.push(lineBuf);
            state.terminal.push("");
        }
        delete[] sortedPrescriptions;
        break;
    }

    case Feature::P_VIEW_BILLS: {
        state.terminal.push("# My Bills");
        double unpaidTotal = 0;
        bool hasBills = false;
        for (int i = 0; i < gBill.size(); i++) {
            Bill& bill = gBill.getAll()[i];
            if (bill.getPatientID() != state.pat->getID()) continue;
            hasBills = true;
            snprintf(lineBuf, 519, "  BillID:%-3d  ApptID:%-3d  PKR%-8.2f  %-10s  %s",
                     bill.getID(), bill.getAppointmentID(),
                     bill.getAmount(), bill.getStatus(), bill.getDate());
            state.terminal.push(lineBuf);
            if (bill.isUnpaid()) unpaidTotal += bill.getAmount();
        }
        if (!hasBills) { state.terminal.push("  No bills."); break; }
        snprintf(lineBuf, 519, "* Outstanding (unpaid): PKR %.2f", unpaidTotal);
        state.terminal.push("");
        state.terminal.push(lineBuf);
        break;
    }

    case Feature::P_PAY_BILL: {
        state.terminal.push("# Pay Bill");
        bool hasUnpaid = false;
        for (int i = 0; i < gBill.size(); i++) {
            Bill& bill = gBill.getAll()[i];
            if (bill.getPatientID() != state.pat->getID() || !bill.isUnpaid()) continue;
            hasUnpaid = true;
            snprintf(lineBuf, 519, "  BillID:%-3d  PKR%-8.2f  %s",
                     bill.getID(), bill.getAmount(), bill.getDate());
            state.terminal.push(lineBuf);
        }
        if (!hasUnpaid) { state.terminal.push("  No unpaid bills."); break; }
        setTerminalPrompt(state, "Bill ID to pay:");
        break;
    }

    case Feature::P_TOPUP: {
        state.terminal.push("# Top Up Balance");
        snprintf(lineBuf, 519, "  Current: PKR %.2f", state.pat->getBalance());
        state.terminal.push(lineBuf);
        setTerminalPrompt(state, "Amount to add (PKR):");
        break;
    }

    case Feature::D_VIEW_TODAY: {
        state.terminal.push("# Today's Appointments");
        char today[11]; getTodayDate(today);
        bool hasAppointments = false;
        for (int i = 0; i < gAppt.size(); i++) {
            Appointment& appt = gAppt.getAll()[i];
            if (appt.getDoctorID() != state.doc->getID() || !strEq(appt.getDate(), today)) continue;
            hasAppointments = true;
            Patient* patient = gPat.findByID(appt.getPatientID());
            snprintf(lineBuf, 519, "  ID:%-3d  %-18s  %-5s  %s",
                     appt.getID(), patient ? patient->getName() : "?",
                     appt.getTimeSlot(), appt.getStatus());
            state.terminal.push(lineBuf);
        }
        if (!hasAppointments) state.terminal.push("  No appointments today.");
        break;
    }

    case Feature::D_MARK_COMPLETE:
    case Feature::D_MARK_NOSHOW: {
        state.terminal.push(feature == Feature::D_MARK_COMPLETE ? "# Mark Complete" : "# Mark No-Show");
        char today[11]; getTodayDate(today);
        for (int i = 0; i < gAppt.size(); i++) {
            Appointment& appt = gAppt.getAll()[i];
            if (appt.getDoctorID() != state.doc->getID() ||
                !strEq(appt.getDate(), today) || !appt.isPending()) continue;
            Patient* patient = gPat.findByID(appt.getPatientID());
            snprintf(lineBuf, 519, "  ID:%-3d  %-18s  %s",
                     appt.getID(), patient ? patient->getName() : "?", appt.getTimeSlot());
            state.terminal.push(lineBuf);
        }
        setTerminalPrompt(state, "Appointment ID:");
        break;
    }

    case Feature::D_WRITE_RX_ID: {
        state.terminal.push("# Write Prescription");
        state.terminal.push("  Today's completed appointments:");
        char today[11]; getTodayDate(today);
        for (int i = 0; i < gAppt.size(); i++) {
            Appointment& appt = gAppt.getAll()[i];
            if (appt.getDoctorID() != state.doc->getID() ||
                !strEq(appt.getDate(), today) || !appt.isCompleted()) continue;
            Patient* patient = gPat.findByID(appt.getPatientID());
            snprintf(lineBuf, 519, "  ID:%-3d  %-18s", appt.getID(), patient ? patient->getName() : "?");
            state.terminal.push(lineBuf);
        }
        setTerminalPrompt(state, "Appointment ID:");
        break;
    }

    case Feature::D_VIEW_HISTORY:
        state.terminal.push("# Patient Medical History");
        setTerminalPrompt(state, "Patient ID:");
        break;

    case Feature::A_VIEW_PATS: {
        state.terminal.push("# All Patients");
        string output = captureOutput([&]() { state.adm->viewAllPatients(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_VIEW_DOCS: {
        state.terminal.push("# All Doctors");
        string output = captureOutput([&]() { state.adm->viewAllDoctors(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_VIEW_APPTS: {
        state.terminal.push("# All Appointments");
        string output = captureOutput([&]() { state.adm->viewAllAppointments(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_VIEW_UNPAID: {
        state.terminal.push("# Unpaid Bills");
        string output = captureOutput([&]() { state.adm->viewUnpaidBills(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_SEC_LOG: {
        state.terminal.push("# Security Log");
        string output = captureOutput([&]() { state.adm->viewSecurityLog(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_DAILY_REPORT: {
        state.terminal.push("# Daily Report");
        string output = captureOutput([&]() { state.adm->generateDailyReport(); });
        state.terminal.pushStr(output);
        break;
    }
    case Feature::A_ADD_DOC:
        state.terminal.push("# Add Doctor  (5 steps)");
        setTerminalPrompt(state, "Doctor name:");
        break;
    case Feature::A_REM_DOC: {
        state.terminal.push("# Remove Doctor");
        string output = captureOutput([&]() { state.adm->viewAllDoctors(); });
        state.terminal.pushStr(output);
        setTerminalPrompt(state, "Doctor ID to remove:");
        break;
    }
    case Feature::A_ADD_PAT:
        state.terminal.push("# Add Patient  (6 steps)");
        setTerminalPrompt(state, "Patient name:");
        break;
    case Feature::A_REM_PAT: {
        state.terminal.push("# Remove Patient");
        string output = captureOutput([&]() { state.adm->viewAllPatients(); });
        state.terminal.pushStr(output);
        setTerminalPrompt(state, "Patient ID to remove:");
        break;
    }
    case Feature::A_DISCHARGE: {
        state.terminal.push("# Discharge Patient");
        string output = captureOutput([&]() { state.adm->viewAllPatients(); });
        state.terminal.pushStr(output);
        setTerminalPrompt(state, "Patient ID to discharge:");
        break;
    }
    default: break;
    }
}

struct MenuItem { const char* label; sf::Color color; Feature feature; };

static const MenuItem PAT_MENU[] = {
    {"1. Book Appointment",      C_BLUE,               Feature::P_BOOK_SPEC},
    {"2. Cancel Appointment",    C_RED,                Feature::P_CANCEL},
    {"3. View My Appointments",  {40, 110, 180, 255},  Feature::P_VIEW_APPTS},
    {"4. View Medical Records",  C_TEAL,               Feature::P_VIEW_RECORDS},
    {"5. View My Bills",         {90, 90, 170, 255},   Feature::P_VIEW_BILLS},
    {"6. Pay Bill",              C_GREEN,              Feature::P_PAY_BILL},
    {"7. Top Up Balance",        C_AMBER,              Feature::P_TOPUP},
};
static const MenuItem DOC_MENU[] = {
    {"1. Today's Appointments",  C_BLUE,               Feature::D_VIEW_TODAY},
    {"2. Mark Complete",         C_GREEN,              Feature::D_MARK_COMPLETE},
    {"3. Mark No-Show",          C_AMBER,              Feature::D_MARK_NOSHOW},
    {"4. Write Prescription",    C_TEAL,               Feature::D_WRITE_RX_ID},
    {"5. Patient History",       {100, 60, 200, 255},  Feature::D_VIEW_HISTORY},
};
static const MenuItem ADM_MENU[] = {
    {" 1. Add Doctor",           C_BLUE,               Feature::A_ADD_DOC},
    {" 2. Remove Doctor",        C_RED,                Feature::A_REM_DOC},
    {" 3. Add Patient",          C_GREEN,              Feature::A_ADD_PAT},
    {" 4. Remove Patient",       {180, 55, 55, 255},   Feature::A_REM_PAT},
    {" 5. View Patients",        {40, 110, 180, 255},  Feature::A_VIEW_PATS},
    {" 6. View Doctors",         C_TEAL,               Feature::A_VIEW_DOCS},
    {" 7. View Appointments",    {75, 75, 180, 255},   Feature::A_VIEW_APPTS},
    {" 8. Unpaid Bills",         C_AMBER,              Feature::A_VIEW_UNPAID},
    {" 9. Discharge Patient",    {140, 75, 145, 255},  Feature::A_DISCHARGE},
    {"10. Security Log",         {50, 115, 95, 255},   Feature::A_SEC_LOG},
    {"11. Daily Report",         {90, 55, 175, 255},   Feature::A_DAILY_REPORT},
};

static void drawRoleMenu(sf::RenderWindow& window, const MenuItem* items, int itemCount,
                         const char* header, const char* subtitle,
                         float mouseX, float mouseY) {
    fillRect(window, 0, 0, (float)WIN_W, 60.f, C_SURF);
    drawHLine(window, 0, 60, (float)WIN_W);
    drawText(window, header, WIN_W / 2.f, 18.f, 18, C_WHITE, true);
    if (subtitle && subtitle[0]) drawText(window, subtitle, 20.f, 68.f, 13, C_GREY);

    const float START_X = 20.f;
    const float START_Y = 90.f;
    const float BUTTON_W = 415.f;
    const float BUTTON_H  = 44.f;
    const float GAP_X = 10.f;
    const float GAP_Y =  8.f;
    const int   COLS = 2;

    for (int i = 0; i < itemCount; i++) {
        int col = i % COLS;
        int row = i / COLS;
        float bx = START_X + col * (BUTTON_W + GAP_X);
        float by = START_Y + row * (BUTTON_H + GAP_Y);
        sf::FloatRect buttonRect{bx, by, BUTTON_W, BUTTON_H};
        drawButton(window, buttonRect, items[i].label, items[i].color,
                   containsPoint(buttonRect, mouseX, mouseY), 14);
    }

    sf::FloatRect logoutRect{WIN_W - 170.f, WIN_H - 56.f, 150.f, 40.f};
    drawButton(window, logoutRect, "Logout", C_RED, containsPoint(logoutRect, mouseX, mouseY), 14);
}

static int getRoleMenuClick(const MenuItem* items, int itemCount, float mouseX, float mouseY) {
    const float START_X  = 20.f;
    const float START_Y  = 90.f;
    const float BUTTON_W = 415.f;
    const float BUTTON_H = 44.f;
    const float GAP_X = 10.f;
    const float GAP_Y =  8.f;
    const int   COLS = 2;

    for (int i = 0; i < itemCount; i++) {
        float bx = START_X + (i % COLS) * (BUTTON_W + GAP_X);
        float by = START_Y + (i / COLS) * (BUTTON_H + GAP_Y);
        if (sf::FloatRect{bx, by, BUTTON_W, BUTTON_H}.contains(mouseX, mouseY)) return i;
    }
    if (sf::FloatRect{WIN_W - 170.f, WIN_H - 56.f, 150.f, 40.f}.contains(mouseX, mouseY)) return itemCount;  // logout
    return -1;
}

static void drawTerminal(sf::RenderWindow& window, const AppState& state,
                         float mouseX, float mouseY) {
    fillRect(window, 0, 0, (float)WIN_W, 52.f, C_SURF);
    drawHLine(window, 0, 52, (float)WIN_W);

    const char* featureTitle = "Terminal";
    switch (state.feature) {
        case Feature::P_BOOK_SPEC: case Feature::P_BOOK_DOCID:
        case Feature::P_BOOK_DATE: case Feature::P_BOOK_SLOT:  featureTitle = "Book Appointment"; break;
        case Feature::P_CANCEL: featureTitle = "Cancel Appointment"; break;
        case Feature::P_VIEW_APPTS: featureTitle = "My Appointments";    break;
        case Feature::P_VIEW_RECORDS: featureTitle = "Medical Records";    break;
        case Feature::P_VIEW_BILLS: featureTitle = "My Bills";           break;
        case Feature::P_PAY_BILL: featureTitle = "Pay Bill";           break;
        case Feature::P_TOPUP: featureTitle = "Top Up Balance";     break;
        case Feature::D_VIEW_TODAY: featureTitle = "Today's Appts";      break;
        case Feature::D_MARK_COMPLETE:featureTitle = "Mark Complete";      break;
        case Feature::D_MARK_NOSHOW:  featureTitle = "Mark No-Show";       break;
        case Feature::D_WRITE_RX_ID: case Feature::D_WRITE_RX_MED:
        case Feature::D_WRITE_RX_NOTE:featureTitle = "Write Prescription"; break;
        case Feature::D_VIEW_HISTORY: featureTitle = "Patient History";    break;
        case Feature::A_ADD_DOC: featureTitle = "Add Doctor";         break;
        case Feature::A_REM_DOC: featureTitle = "Remove Doctor";      break;
        case Feature::A_ADD_PAT: featureTitle = "Add Patient";        break;
        case Feature::A_REM_PAT: featureTitle = "Remove Patient";     break;
        case Feature::A_VIEW_PATS: featureTitle = "All Patients";       break;
        case Feature::A_VIEW_DOCS: featureTitle = "All Doctors";        break;
        case Feature::A_VIEW_APPTS: featureTitle = "All Appointments";   break;
        case Feature::A_VIEW_UNPAID: featureTitle = "Unpaid Bills";       break;
        case Feature::A_DISCHARGE: featureTitle = "Discharge Patient";  break;
        case Feature::A_SEC_LOG: featureTitle = "Security Log";       break;
        case Feature::A_DAILY_REPORT: featureTitle = "Daily Report";       break;
        default: break;
    }
    drawText(window, featureTitle, WIN_W / 2.f, 16.f, 17, C_TEAL, true);

    const float PANEL_X = 14.f;
    const float PANEL_Y = 58.f;
    const float PANEL_W = WIN_W - 28.f;
    const float PANEL_H = 440.f;
    fillRect(window, PANEL_X, PANEL_Y, PANEL_W, PANEL_H, C_INP, C_BORDER, 1.f);

    const int   VISIBLE_LINES = 20;
    const float LINE_HEIGHT   = 21.f;

    for (int i = 0; i < VISIBLE_LINES; i++) {
        int lineIndex = state.terminal.scrollOffset + i;
        if (lineIndex >= state.terminal.lineCount) break;

        const char* line = state.terminal.lines[lineIndex];
        sf::Color lineColor = C_WHITE;

        if (line[0] == '!' || line[0] == 'E') lineColor = C_RED;
        else if (line[0] == '>' || line[0] == '*') lineColor = C_GREEN;
        else if (line[0] == '#') lineColor = C_AMBER;
        else if (line[0] == ' ' && line[1] == ' ') lineColor = C_GREY;

        drawText(window, line, PANEL_X + 8.f, PANEL_Y + 4.f + i * LINE_HEIGHT, 13, lineColor);
    }

    if (state.terminal.lineCount > VISIBLE_LINES) {
        float scrollFraction = (float)state.terminal.scrollOffset /
                               (float)std::max(1, state.terminal.lineCount - VISIBLE_LINES);
        float scrollbarHeight = PANEL_H * (float)VISIBLE_LINES / (float)state.terminal.lineCount;
        float scrollbarY      = PANEL_Y + scrollFraction * (PANEL_H - scrollbarHeight);
        fillRect(window, PANEL_X + PANEL_W - 7.f, scrollbarY, 5.f, scrollbarHeight, C_SURF2);
    }

    const float INPUT_Y = PANEL_Y + PANEL_H + 12.f;
    if (!state.prompt.empty()) {
        drawText(window, state.prompt, PANEL_X, INPUT_Y - 16.f, 12, C_GREY);
        sf::FloatRect inputFieldRect{PANEL_X, INPUT_Y, PANEL_W - 160.f, 36.f};
        drawInputBox(window, inputFieldRect, state.inputText, true, state.inputIsPassword);
        sf::FloatRect submitButtonRect{PANEL_X + PANEL_W - 152.f, INPUT_Y, 142.f, 36.f};
        drawButton(window, submitButtonRect, "Submit", C_BLUE,
                   containsPoint(submitButtonRect, mouseX, mouseY), 14);
    }

    drawText(window, "Mouse wheel / PageUp / PageDown to scroll",
             PANEL_X, (float)WIN_H - 20.f, 11, C_DARK);
    sf::FloatRect backButtonRect{WIN_W - 160.f, (float)WIN_H - 50.f, 142.f, 36.f};
    drawButton(window, backButtonRect, "< Back", C_SURF2,
               containsPoint(backButtonRect, mouseX, mouseY), 14);
}

static void drawLauncher(sf::RenderWindow& window, float mouseX, float mouseY) {
    fillRect(window, 0, 0, (float)WIN_W, 70.f, C_SURF);
    drawHLine(window, 0, 70, (float)WIN_W);
    drawText(window, "MediCore Hospital Management System",
             WIN_W / 2.f, 22.f, 20, C_TEAL, true);

    const float BUTTON_W = 190.f;
    const float BUTTON_H = 58.f;
    const float GAP = 18.f;
    const float totalButtonWidth = 4.f * BUTTON_W + 3.f * GAP;
    const float startX = (WIN_W - totalButtonWidth) / 2.f;
    const float startY = 130.f;

    struct { const char* label; sf::Color color; } roleButtons[4] = {
        {"Patient", C_BLUE}, {"Doctor", C_TEAL},
        {"Admin", {100, 60, 200, 255}}, {"Exit", C_RED}
    };
    for (int i = 0; i < 4; i++) {
        sf::FloatRect buttonRect{startX + i * (BUTTON_W + GAP), startY, BUTTON_W, BUTTON_H};
        drawButton(window, buttonRect, roleButtons[i].label, roleButtons[i].color,
                   containsPoint(buttonRect, mouseX, mouseY), 17);
    }

    const float featuresY = 220.f;
    fillRect(window, 40.f, featuresY, WIN_W - 80.f, 270.f, C_SURF, C_BORDER, 1.f);
    drawText(window, "System Features", WIN_W / 2.f, featuresY + 14.f, 15, C_AMBER, true);
    drawHLine(window, 60.f, featuresY + 38.f, WIN_W - 120.f);

    const char* featureLabels[] = {
        "Appointment Booking & Cancellation",
        "Prescription Writing & Records",
        "Billing & Payment Processing",
        "Doctor & Patient Administration",
        "Security Audit Logging",
        "Daily Revenue Reports"
    };
    for (int i = 0; i < 6; i++) {
        float colX = (i < 3) ? 80.f : WIN_W / 2.f + 10.f;
        float rowY = featuresY + 52.f + (i % 3) * 44.f;
        drawText(window, "*", colX - 18.f, rowY, 11, C_TEAL);
        drawText(window, featureLabels[i], colX, rowY, 13, C_GREY);
    }
}

static int getLauncherClick(float mouseX, float mouseY) {
    const float BUTTON_W = 190.f;
    const float BUTTON_H = 58.f;
    const float GAP = 18.f;
    const float totalButtonWidth = 4.f * BUTTON_W + 3.f * GAP;
    const float startX = (WIN_W - totalButtonWidth) / 2.f;
    const float startY = 130.f;
    for (int i = 0; i < 4; i++) {
        sf::FloatRect buttonRect{startX + i * (BUTTON_W + GAP), startY, BUTTON_W, BUTTON_H};
        if (buttonRect.contains(mouseX, mouseY)) return i + 1;
    }
    return 0;
}

static const char* getRoleLabel(int role) {
    if (role == 1) return "Patient";
    if (role == 2) return "Doctor";
    return "Admin";
}

static sf::Color getRoleColor(int role) {
    if (role == 1) return C_BLUE;
    if (role == 2) return C_TEAL;
    return sf::Color{100, 60, 200, 255};
}

static void drawLogin(sf::RenderWindow& window, const AppState& state,
                      float mouseX, float mouseY) {
    fillRect(window, 0, 0, (float)WIN_W, 60.f, C_SURF);
    drawHLine(window, 0, 60, (float)WIN_W);
    char headerText[64];
    snprintf(headerText, 63, "Login  -  %s", getRoleLabel(state.role));
    drawText(window, headerText, WIN_W / 2.f, 18.f, 18, getRoleColor(state.role), true);

    const float CARD_X = WIN_W / 2.f - 200.f;
    const float CARD_Y = 90.f;
    const float CARD_W = 400.f;
    const float CARD_H = 320.f;
    fillRect(window, CARD_X, CARD_Y, CARD_W, CARD_H, C_SURF, C_BORDER, 1.f);

    drawText(window, "ID", CARD_X + 28.f, CARD_Y + 24.f, 13, C_GREY);
    sf::FloatRect idInputRect{CARD_X + 28.f, CARD_Y + 44.f, 344.f, 40.f};
    drawInputBox(window, idInputRect, state.loginId, state.idFieldFocused);

    drawText(window, "Password", CARD_X + 28.f, CARD_Y + 100.f, 13, C_GREY);
    sf::FloatRect passwordInputRect{CARD_X + 28.f, CARD_Y + 120.f, 344.f, 40.f};
    drawInputBox(window, passwordInputRect, state.loginPassword, !state.idFieldFocused, true);

    if (!state.loginError.empty())
        drawText(window, state.loginError, CARD_X + 28.f, CARD_Y + 176.f, 13, C_RED);

    drawText(window, "Tab = switch field   Enter = submit",
             CARD_X + 28.f, CARD_Y + 200.f, 11, C_DARK);

    sf::FloatRect loginButtonRect{CARD_X + 28.f,  CARD_Y + 230.f, 156.f, 42.f};
    sf::FloatRect backButtonRect {CARD_X + 204.f, CARD_Y + 230.f, 156.f, 42.f};

    drawButton(window, loginButtonRect, "Login", getRoleColor(state.role),
               containsPoint(loginButtonRect, mouseX, mouseY), 15);
    drawButton(window, backButtonRect,  "Back",  C_SURF2,
               containsPoint(backButtonRect,  mouseX, mouseY), 15);
}

static bool loadFont() {
    const char* paths[] = {
        "Fonts/data-latin.ttf",        "Fonts\\data-latin.ttf",
        "Fonts/ArcadeClassic.ttf",     "Fonts\\ArcadeClassic.ttf",
        "C:/Users/rayya/Downloads/25L_0878_Proejct Hospital/Fonts/data-latin.ttf",
        "C:/Users/rayya/Downloads/25L_0878_Proejct Hospital/Fonts/ArcadeClassic.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        nullptr
    };
    for (int i = 0; paths[i]; i++) {
        if (gFont.loadFromFile(paths[i])) return true;
    }
    return false;
}

int main() {
    try {
        fileHandler::loadPatients     (gPat);
        fileHandler::loadDoctors      (gDoc);
        fileHandler::loadAppointments (gAppt);
        fileHandler::loadBills        (gBill);
        fileHandler::loadPrescriptions(gPresc);
    } catch (...) {}

    {
        ifstream adminFile("admin.txt");
        if (adminFile.is_open()) {
            char line[200];
            adminFile.getline(line, 200);  // skip header
            while (adminFile.getline(line, 200)) {
                char* token = strtok(line, ",");

                if (!token) continue;

                int adminId = atoi(token);

                token = strtok(NULL, ",");

                if (!token) continue;

                int nameLen = (int)strlen(token);
                char* adminName = new char[nameLen + 1];

                memcpy(adminName, token, nameLen + 1);

                token = strtok(NULL, ",");
                
                if (!token) { 
                    delete[] adminName; 
                    continue; 
                }
                int passLen = (int)strlen(token);

                char* adminPassword = new char[passLen + 1];
                
                memcpy(adminPassword, token, passLen + 1);

                Admin adminUser(adminId, adminName, adminPassword);

                gAdm.add(adminUser);

                delete[] adminName;
                delete[] adminPassword;
            }
            adminFile.close();
        }
    }
    syncBillCounts();

    sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "MediCore");
    window.setFramerateLimit(60);

    if (!loadFont()) {
        window.clear(C_BG);
        fillRect(window, 100.f, 220.f, 700.f, 100.f, {60, 10, 10, 255}, {200, 40, 40, 255}, 2.f);
        window.display();
        fprintf(stderr, "[MediCore] ERROR: no font loaded.\nCopy the Fonts\\ folder next to the .exe.\n");
        fflush(stderr);
        sf::Clock timer;
        while (window.isOpen() && timer.getElapsedTime().asSeconds() < 6.f) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) window.close();
            }
        }
        return 1;
    }

    AppState state;

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float mouseX = (float)mousePos.x;
        float mouseY = (float)mousePos.y;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::TextEntered) {
                sf::Uint32 unicodeChar = event.text.unicode;
                if (unicodeChar >= 32 && unicodeChar < 127) {
                    char typedChar = (char)unicodeChar;
                    if (state.screen == Screen::LOGIN) {
                        if (state.idFieldFocused && state.loginId.size() < 15) {
                            state.loginId.push_back(typedChar);
                        }
                        else if (!state.idFieldFocused && state.loginPassword.size() < 63) {
                            state.loginPassword.push_back(typedChar);
                        }
                    } 
                    else if (state.screen == Screen::TERMINAL && !state.prompt.empty()) {
                        if (state.inputText.size() < 511) {
                            state.inputText.push_back(typedChar);
                        }
                    }
                }
            }

            if (event.type == sf::Event::KeyPressed) {
                using Key = sf::Keyboard;
                auto keyCode = event.key.code;

                if (keyCode == Key::BackSpace) {
                    if (state.screen == Screen::LOGIN) {
                        if (state.idFieldFocused && !state.loginId.empty()) {
                            state.loginId.pop_back();
                        }
                        else if (!state.idFieldFocused && !state.loginPassword.empty()) {
                            state.loginPassword.pop_back();
                        }
                    } 
                    else if (state.screen == Screen::TERMINAL && !state.inputText.empty()) {
                        state.inputText.pop_back();
                    }
                }
                if (keyCode == Key::Tab && state.screen == Screen::LOGIN)
                    state.idFieldFocused = !state.idFieldFocused;
                if (keyCode == Key::Return) {
                    if (state.screen == Screen::LOGIN) attemptLogin(state);
                    if (state.screen == Screen::TERMINAL && !state.prompt.empty()) onSubmit(state);
                }
                if (keyCode == Key::Escape) {
                    if (state.screen == Screen::TERMINAL) {
                        state.screen = Screen::ROLE;
                    } 
                    else if (state.screen == Screen::ROLE) {
                        state.screen = Screen::LAUNCHER;
                        state.role = 0;
                        state.pat = nullptr;
                        state.doc = nullptr;
                        state.adm = nullptr;
                    } 
                    else if (state.screen == Screen::LOGIN) {
                        state.screen = Screen::LAUNCHER;
                    }
                }
                if (state.screen == Screen::TERMINAL) {
                    if (keyCode == Key::PageUp)   state.terminal.scrollUp(4);
                    if (keyCode == Key::PageDown)  state.terminal.scrollDown(4);
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled &&
                state.screen == Screen::TERMINAL) {
                if (event.mouseWheelScroll.delta > 0) state.terminal.scrollUp(2);
                else state.terminal.scrollDown(2);
            }

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                float clickX = (float)event.mouseButton.x;
                float clickY = (float)event.mouseButton.y;

                if (state.screen == Screen::LAUNCHER) {
                    int roleChoice = getLauncherClick(clickX, clickY);
                    if (roleChoice == 4) {
                        window.close();
                    } else if (roleChoice > 0) {
                        state.role          = roleChoice;
                        state.screen        = Screen::LOGIN;
                        state.loginId.clear();
                        state.loginPassword.clear();
                        state.loginError    = "";
                        state.idFieldFocused = true;
                    }
                } 
                else if (state.screen == Screen::LOGIN) {
                    const float CARD_X = WIN_W / 2.f - 200.f;
                    const float CARD_Y = 90.f;
                    if (sf::FloatRect{CARD_X + 28.f, CARD_Y + 44.f,  344.f, 40.f}.contains(clickX, clickY))
                        state.idFieldFocused = true;
                    if (sf::FloatRect{CARD_X + 28.f, CARD_Y + 120.f, 344.f, 40.f}.contains(clickX, clickY))
                        state.idFieldFocused = false;
                    if (sf::FloatRect{CARD_X + 28.f, CARD_Y + 230.f, 156.f, 42.f}.contains(clickX, clickY))
                        attemptLogin(state);
                    if (sf::FloatRect{CARD_X + 204.f, CARD_Y + 230.f, 156.f, 42.f}.contains(clickX, clickY))
                        state.screen = Screen::LAUNCHER;

                } 
                else if (state.screen == Screen::ROLE) {
                    int menuIndex = -1;
                    if      (state.role == 1) menuIndex = getRoleMenuClick(PAT_MENU, 7,  clickX, clickY);
                    else if (state.role == 2) menuIndex = getRoleMenuClick(DOC_MENU, 5,  clickX, clickY);
                    else                      menuIndex = getRoleMenuClick(ADM_MENU, 11, clickX, clickY);

                    int menuSize = (state.role == 1) ? 7 : (state.role == 2) ? 5 : 11;
                    if (menuIndex == menuSize) {
                        state.screen = Screen::LAUNCHER;
                        state.role = 0;
                        state.pat  = nullptr;
                        state.doc  = nullptr;
                        state.adm  = nullptr;
                    } 
                    else if (menuIndex >= 0) {
                        if      (state.role == 1) launchFeature(state, PAT_MENU[menuIndex].feature);
                        else if (state.role == 2) launchFeature(state, DOC_MENU[menuIndex].feature);
                        else                      launchFeature(state, ADM_MENU[menuIndex].feature);
                    }

                } 
                else if (state.screen == Screen::TERMINAL) {
                    const float PANEL_X  = 14.f;
                    const float PANEL_Y  = 58.f;
                    const float PANEL_W  = WIN_W - 28.f;
                    const float PANEL_H  = 440.f;
                    const float INPUT_Y  = PANEL_Y + PANEL_H + 12.f;

                    if (!state.prompt.empty() &&
                        sf::FloatRect{PANEL_X + PANEL_W - 152.f, INPUT_Y, 142.f, 36.f}.contains(clickX, clickY))
                        onSubmit(state);

                    if (sf::FloatRect{WIN_W - 160.f, (float)WIN_H - 50.f, 142.f, 36.f}.contains(clickX, clickY))
                        state.screen = Screen::ROLE;
                }
            }
        }

        window.clear(C_BG);
        switch (state.screen) {
            case Screen::LAUNCHER: drawLauncher(window, mouseX, mouseY); break;
            case Screen::LOGIN:    drawLogin(window, state, mouseX, mouseY); break;
            case Screen::ROLE: {
                char subtitle[128] = "";
                if (state.role == 1) {
                    snprintf(subtitle, 127, "Welcome, %s   |   Balance: PKR %.2f", state.pat->getName(), state.pat->getBalance());
                }
                    
                else if (state.role == 2) {
                    snprintf(subtitle, 127, "Dr. %s   |   %s",state.doc->getName(), state.doc->getSpecialization());
                }
                    
                const char* header = (state.role == 1) ? "Patient Menu"
                                          : (state.role == 2) ? "Doctor Menu" : "Admin Panel";

                const MenuItem* items = (state.role == 1) ? PAT_MENU
                                          : (state.role == 2) ? DOC_MENU : ADM_MENU;

                int itemCount = (state.role == 1) ? 7 : (state.role == 2) ? 5 : 11;

                drawRoleMenu(window, items, itemCount, header, subtitle, mouseX, mouseY);

                break;
            }
            case Screen::TERMINAL: drawTerminal(window, state, mouseX, mouseY); break;
        }
        window.display();
    }
}