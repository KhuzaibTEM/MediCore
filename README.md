<img width="910" height="660" alt="Screenshot 2026-05-11 002059" src="https://github.com/user-attachments/assets/34a01afd-61ba-4197-830a-cee317dddbbd" />
# MediCore

## Overview
A small C++ clinic/hospital management system with both GUI and console components. It manages patients, doctors, appointments, billing, prescriptions, discharge records, and a security log. Data is stored in plain text files in the project folder so the system is lightweight and easy to inspect.

## Key Features
- **Patients:** add, view, edit, and store patient records (data in `patients.txt`).
- **Appointments:** patients can make appointments; appointments are stored in `appointments.txt`.
- **Doctors:** manage doctor records and availability (`doctors.txt`).
- **Billing:** generate and store bills; billing records in `bills.txt`.
- **Prescriptions:** create and save prescriptions (`prescriptions.txt`).
- **Discharge:** record discharged patients in `discharged.txt`.
- **Security Log:** actions and security-related events are appended to `security_log.txt`.
- **GUI:** a graphical interface entry point exists (`gui_main.cpp` / `clinic_gui.exe`).

## Project Structure (important files)
- [main.cpp](main.cpp) — program entry (console flows).
- [gui_main.cpp](gui_main.cpp) — GUI entry point.
- [Doctor.cpp](Doctor.cpp), [Doctor.h](Doctor.h) — doctor model/logic.
- [admin.cpp](admin.cpp), [admin.h](admin.h) — admin features.
- [Patient.h](Patient.h), [Person.h](Person.h) — patient/person models.
- [Bill.h](Bill.h) — billing model.
- [Prescription.h](Prescription.h) — prescription model.
- [Filehandler.h](Filehandler.h), [storage.h](storage.h) — file I/O and persistence.
- [Validator.h](Validator.h), [Exceptions.h](Exceptions.h) — validation and error handling.
- Data files: [patients.txt](patients.txt), [doctors.txt](doctors.txt), [appointments.txt](appointments.txt), [bills.txt](bills.txt), [prescriptions.txt](prescriptions.txt), [discharged.txt](discharged.txt), [security_log.txt](security_log.txt).
- Assets: `assets/`, `Fonts/` — GUI resources.

## Build & Run
- Build with the provided `Makefile` (requires a compatible C++ toolchain):

```bash
make
```

- On Windows, if a prebuilt executable exists, run:

```powershell
.\clinic_gui.exe
```

- Or run the console app (if built):

```bash
./clinic_gui.exe
# or
./main.exe
```

Notes: The project targets standard C++ compilation; adjust compiler flags as needed. The GUI executable depends only on the project sources and bundled assets.

## Data Storage
All application data is persisted as plain text files in the project folder (one file per entity). This makes it easy to back up, edit, or migrate data manually.

## Typical Workflows
- A patient record is created and stored in `patients.txt`.
- The patient or staff creates an appointment; the appointment is appended to `appointments.txt`.
- Doctors add prescriptions, which are saved to `prescriptions.txt`.
- When a patient is billed, an entry is added to `bills.txt`.
- When discharged, the patient's discharge information is saved in `discharged.txt`.
- Administrative actions and notable events are logged to `security_log.txt`.


