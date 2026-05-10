#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "constants.h"
#include <ctime>
#include <iostream>
using namespace std;

class Validator {
private:

    static bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    static int strLen(const char* s) {
        int i = 0;
        while (s[i] != '\0') i++;
        return i;
    }

    static bool strEq(const char* a, const char* b) {
        int i = 0;
        while (a[i] != '\0' && b[i] != '\0') {
            if (a[i] != b[i]) return false;
            i++;
        }
        return (a[i] == '\0' && b[i] == '\0');
    }

    static int digitVal(char c) { return c - '0'; }

    static int getCurrentYear() {
        time_t now = time(0);
        tm* t = localtime(&now);
        return 1900 + t->tm_year;
    }

public:

    static bool validateID(int id) {
        return id > 0;
    }

    static bool validateDate(const char* date) {
        if (strLen(date) != 10) return false;

        if (date[2] != '-' || date[5] != '-') return false;

        for (int i = 0; i < 10; i++) {
            if (i == 2 || i == 5) continue;
            if (!isDigit(date[i])) return false;
        }

        int day   = digitVal(date[0]) * 10 + digitVal(date[1]);
        int month = digitVal(date[3]) * 10 + digitVal(date[4]);
        int year  = digitVal(date[6]) * 1000 + digitVal(date[7]) * 100
                  + digitVal(date[8]) * 10   + digitVal(date[9]);

        if (day < 1 || day > 31)       return false;
        if (month < 1 || month > 12)   return false;
        if (year < getCurrentYear())   return false;

        return true;
    }

    /*
     * Must be one of the 8 fixed daily slots (HH:MM, 24-hour).
     */
    static bool validateTimeSlot(const char* slot) {
        const char* valid[8] = {
            "09:00", "10:00", "11:00", "12:00",
            "13:00", "14:00", "15:00", "16:00"
        };
        for (int i = 0; i < 8; i++) {
            if (strEq(slot, valid[i])) return true;
        }
        return false;
    }

    static bool validateContact(const char* contact) {
        if (strLen(contact) != 11) return false;
        for (int i = 0; i < 11; i++) {
            if (!isDigit(contact[i])) return false;
        }
        return true;
    }

    /*
     * Minimum 6 characters, maximum 30 characters, non-empty.
     */
    static bool validatePassword(const char* password) {
        int len = strLen(password);
        return len >= 6 && len <= 30;
    }

    static bool validatePositiveFloat(double value) {
        return value > 0.0;
    }

    static bool validateMenuChoice(int choice, int min, int max) {
        return choice >= min && choice <= max;
    }

    static bool validateAge(int age) {
        return age >= 1 && age <= 180;
    }

    static bool validateGender(char gender) {
        return gender == 'M' || gender == 'F';
    }

    static bool validateSpecialization(const char* s) {
        return strLen(s) > 0;
    }

    static bool validateName(const char* name) {
        int len = strLen(name);
        if (len == 0) return false;
        for (int i = 0; i < len; i++) {
            char c = name[i];
            bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
            bool space  = (c == ' ');
            bool dot    = (c == '.');   // for "Dr. X" style names
            if (!letter && !space && !dot) return false;
        }
        return true;
    }
};

#endif