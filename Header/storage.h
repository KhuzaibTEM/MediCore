#ifndef STORAGE_H
#define STORAGE_H


template <typename T>
class Storage {
    private:
        T data[100];
        int count;

    public:
        Storage() {
            count = 0;
        }

        bool add(const T& obj) {
            if (count >= 100) return false;

            data[count] = obj;
            count++;

            return true;
        }

        bool removeByID(int _id) {
            int index = -1;

            for (int i = 0; i < count; i++) {
                if (data[i].getID() == _id) {
                    index = i;
                    break;
                }
            }

            if (index == -1) return false;

            for (int i = index; i < count - 1; i++) {
                data[i] = data[i + 1];
            }

            count--;

            return true;
        }

        T* findByID(int _id) {
            for (int i = 0; i < count; i++) {
                if (data[i].getID() == _id) {
                    return &data[i];
                }
            }

            return nullptr;
        }

        T* getAll() {
            return data;
        }

        int size() const {
            return count;
        }
};

#endif