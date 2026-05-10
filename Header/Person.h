#ifndef PERSON_H
#define PERSON_H

#include "constants.h"

class Person {
    protected:
        int id;
        char* name;
        char* password;

    public:
        Person() : id(0) {
            name = new char[1];
            name[0] = '\0';

            password = new char[1];
            password[0] = '\0';
        }
        Person(int _id, char * _name, char * _password) {
            int size_n = lenArr(_name);
            int size_p = lenArr(_password);

            id = _id;
            name = new char[size_n];
            password = new char[size_p];

            for (int i = 0; i <= size_n; i++) {
                name[i] = _name[i];
            }
            for (int i = 0; i <= size_p; i++) {
                password[i] = _password[i];
            }
        }

        Person(const Person& obj) {
            int size_n = lenArr(obj.name);
            int size_p = lenArr(obj.password);

            id = obj.id;
            name = new char[size_n + 1];
            password = new char[size_p + 1];

            for (int i = 0; i <= size_n; i++) {
                name[i] = obj.name[i];
            }
            for (int i = 0; i <= size_p; i++) {
                password[i] = obj.password[i];
            }
        }

        Person& operator=(const Person& obj) {
            
            if (this == &obj) return *this;

            delete[] name;
            delete[] password;

            int size_n = lenArr(obj.name);
            int size_p = lenArr(obj.password);

            id = obj.id;
            name = new char[size_n + 1];
            password = new char[size_p + 1];

            for (int i = 0; i <= size_n; i++) {
                name[i] = obj.name[i];
            }
            for (int i = 0; i <= size_p; i++) {
                password[i] = obj.password[i];
            }
            return *this;
        }

        virtual ~Person() {
            delete[] name;
            delete[] password;
        }

        virtual void display() {
            cout << "ID: " << id;
            cout << " | Name: " << name;
        }

        virtual char* getPassword() {
            return password;
        }
};

#endif