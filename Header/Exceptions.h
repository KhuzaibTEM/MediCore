#ifndef HOSPITAL_EXCEPTION_H
#define HOSPITAL_EXCEPTION_H

#include <iostream>
using namespace std;

class HospitalException {
    protected:
        char message[200];

        static void copyMsg(char* dst, const char* src) {
            int i = 0;
            while (src[i] != '\0' && i < 199) { dst[i] = src[i]; i++; }
            dst[i] = '\0';
        }

    public:
        HospitalException() { 
            message[0] = '\0'; 
        }

        explicit HospitalException(const char* msg) {
            copyMsg(message, msg);
        }

        virtual const char* what() const { 
            return message; 
        }

        virtual ~HospitalException() {}
};

class FileNotFoundException : public HospitalException {
    public:
        explicit FileNotFoundException(const char* filename)
            : HospitalException("") {
            char buffer[200] = "File not found: ";

            int bi = 16;
            for (int i = 0; filename[i] != '\0' && bi < 199; i++) buffer[bi++] = filename[i];

            buffer[bi] = '\0';
            copyMsg(message, buffer);
        }
};

class InsufficientFundsException : public HospitalException {
    public:
        explicit InsufficientFundsException(const char* msg = "Insufficient funds.")
            : HospitalException(msg) {}
};

class InvalidInputException : public HospitalException {
    public:
        explicit InvalidInputException(const char* msg = "Invalid input.")
            : HospitalException(msg) {}
};

class SlotUnavailableException : public HospitalException {
    public:
        explicit SlotUnavailableException(const char* slot)
            : HospitalException("") {

            char buffer[60] = "Time slot ";

            int bi = 10;
            for (int i = 0; slot[i] != '\0' && bi < 59; i++) buffer[bi++] = slot[i];

            const char* sfx = " is already booked.";

            for (int i = 0; sfx[i] != '\0' && bi < 59; i++) buffer[bi++] = sfx[i];

            buffer[bi] = '\0';

            copyMsg(message, buffer);
        }
};

#endif