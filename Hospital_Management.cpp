#include <iostream>
#include <windows.h>
#include <conio.h>
#include <string>
#include <fstream>
#include <cctype>
#include <limits>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>

using namespace std;

// Maximum number of users supported by the system
const int UserSize = 100;

using StrArray = string[UserSize];
using IntArray = int[UserSize];

const char FIELD_DELIMITER = '\t';
const string PATIENTS_FILE = "patients.txt";
const string APPOINTMENTS_FILE = "appointments.txt";
const string DOCTORS_FILE = "doctors.txt";
const string RECEPTIONISTS_FILE = "receptionists.txt";
const string PHARMACISTS_FILE = "pharmacists.txt";
const string MEDICINES_FILE = "medicines.txt";
const string PATIENT_HISTORY_FILE = "patient_history.txt";
const string DOCTOR_REQUESTS_FILE = "doctor_requests.txt";
const string DOCTOR_RATINGS_FILE = "doctor_ratings.txt";
const string MEDICINE_EXPIRY_FILE = "medicine_expiry.txt";
const string REVENUE_FILE = "revenue.txt";
const int APPOINTMENT_SLOT_MINUTES = 15;
const int CONSULTATION_FEE = 1500;
const int EMERGENCY_CONSULTATION_FEE = 2500;

int SplitFields(const string &line, string fields[], int maxFields)
{
    int actualCount = 0;
    string current;
    for (char c : line)
    {
        if (c == FIELD_DELIMITER)
        {
            if (actualCount < maxFields)
            {
                fields[actualCount] = current;
            }
            current.clear();
            actualCount++;
        }
        else
        {
            current += c;
        }
    }

    if (actualCount < maxFields)
    {
        fields[actualCount] = current;
    }
    actualCount++;

    if (actualCount > maxFields)
    {
        actualCount = maxFields;
    }

    return actualCount;
}

string GetField(const string fields[], int count, int index)
{
    if (index < count)
    {
        return fields[index];
    }
    return "";
}

string ToUpper(const string &value)
{
    string result = value;
    for (char &c : result)
    {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

int FindDoctorIndex(const StrArray &Doctors, int DoctorCount, const string &doctorName)
{
    for (int i = 0; i < DoctorCount; i++)
    {
        if (Doctors[i] == doctorName)
        {
            return i;
        }
    }
    return -1;
}

int ParseTimeToMinutes(const string &timeStr)
{
    string trimmed;
    for (char c : timeStr)
    {
        if (!isspace(static_cast<unsigned char>(c)))
        {
            trimmed += c;
        }
    }
    if (trimmed.empty())
    {
        return -1;
    }

    string suffix;
    if (trimmed.size() >= 2)
    {
        char secondLast = static_cast<char>(toupper(static_cast<unsigned char>(trimmed[trimmed.size() - 2])));
        char last = static_cast<char>(toupper(static_cast<unsigned char>(trimmed[trimmed.size() - 1])));
        if ((secondLast == 'A' || secondLast == 'P') && last == 'M')
        {
            suffix = string(1, secondLast) + last;
            trimmed.erase(trimmed.size() - 2);
        }
    }

    int hours = 0;
    int minutes = 0;
    size_t colonPos = trimmed.find(':');
    try
    {
        if (colonPos != string::npos)
        {
            hours = stoi(trimmed.substr(0, colonPos));
            minutes = stoi(trimmed.substr(colonPos + 1));
        }
        else
        {
            hours = stoi(trimmed);
        }
    }
    catch (...)
    {
        return -1;
    }

    hours = hours % 24;
    minutes = minutes % 60;

    if (suffix == "PM" && hours < 12)
    {
        hours += 12;
    }
    else if (suffix == "AM" && hours == 12)
    {
        hours = 0;
    }

    return hours * 60 + minutes;
}

string FormatMinutesToTime(int minutes)
{
    if (minutes < 0)
    {
        return "";
    }
    minutes %= (24 * 60);
    int hours = minutes / 60;
    int mins = minutes % 60;
    string formatted;
    formatted += (hours < 10 ? "0" : "");
    formatted += to_string(hours);
    formatted += ':';
    formatted += (mins < 10 ? "0" : "");
    formatted += to_string(mins);
    return formatted;
}

string PromptLine(const string &message)
{
    cout << message;
    string value;
    getline(cin >> ws, value);
    return value;
}

int CountAppointmentsForDoctorOnDate(const StrArray &appointmentDoctor, const StrArray &appointmentDate,
                                     int appointmentCount, const string &doctor, const string &date)
{
    int count = 0;
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentDoctor[i] == doctor && appointmentDate[i] == date)
        {
            count++;
        }
    }
    return count;
}

int ExtractNumericPrice(const string &price)
{
    int value = 0;
    for (char c : price)
    {
        if (isdigit(static_cast<unsigned char>(c)))
        {
            value = value * 10 + (c - '0');
        }
    }
    return value;
}

int SafeStringToInt(const string &value, int fallback)
{
    try
    {
        return stoi(value);
    }
    catch (...)
    {
        return fallback;
    }
}

int FindMedicineIndex(const StrArray &Medicines, int MedicineCount, const string &medicineName)
{
    string target = ToUpper(medicineName);
    for (int i = 0; i < MedicineCount; i++)
    {
        if (ToUpper(Medicines[i]) == target)
        {
            return i;
        }
    }
    return -1;
}

int FindMedicinePrice(const StrArray &Medicines, const StrArray &MedicinePrices, int MedicineCount, const string &medicineName)
{
    int index = FindMedicineIndex(Medicines, MedicineCount, medicineName);
    if (index == -1)
    {
        return 0;
    }
    return ExtractNumericPrice(MedicinePrices[index]);
}

string SanitizeFileName(const string &name)
{
    string sanitized;
    for (char c : name)
    {
        if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')
        {
            sanitized += c;
        }
    }
    return sanitized;
}

string EscapePDFText(const string &text)
{
    string escaped;
    for (char c : text)
    {
        if (c == '(' || c == ')' || c == '\\')
        {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}

bool GeneratePatientBillPDF(const string &patientName,
                            const StrArray &appointmentPatient,
                            const StrArray &PrescriptionMed,
                            int appointmentCount,
                            const StrArray &Medicines,
                            const StrArray &MedicinePrices,
                            int MedicineCount)
{
    string normalized = ToUpper(patientName);
    string medicineNames[UserSize];
    int medicineLinePrices[UserSize] = {};
    int medicineLineCount = 0;
    int medicineTotal = 0;

    for (int i = 0; i < appointmentCount && medicineLineCount < UserSize; i++)
    {
        if (ToUpper(appointmentPatient[i]) == normalized && !PrescriptionMed[i].empty())
        {
            int price = FindMedicinePrice(Medicines, MedicinePrices, MedicineCount, PrescriptionMed[i]);
            medicineTotal += price;
            medicineNames[medicineLineCount] = PrescriptionMed[i];
            medicineLinePrices[medicineLineCount] = price;
            medicineLineCount++;
        }
    }

    if (medicineLineCount == 0)
    {
        medicineNames[medicineLineCount] = "No medicines prescribed.";
        medicineLinePrices[medicineLineCount] = 0;
        medicineLineCount++;
    }

    int total = CONSULTATION_FEE + medicineTotal;

    auto formatCurrency = [](int value) {
        return string("Rs.") + to_string(value);
    };

    time_t now = time(nullptr);
    tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localTime = *localtime(&now);
#endif
    ostringstream dateStream;
    dateStream << put_time(&localTime, "%Y-%m-%d %H:%M");
    string issuedOn = dateStream.str();

    ostringstream invoiceStream;
    invoiceStream << "INV-" << put_time(&localTime, "%Y%m%d%H%M");
    string invoiceId = invoiceStream.str();

    ostringstream content;
    
    // Draw header box
    content << "q\n";
    content << "0.9 0.9 0.9 rg\n";
    content << "50 730 512 50 re f\n";
    content << "Q\n";
    content << "0.5 w\n";
    content << "50 730 512 50 re S\n";
    
    // Header text
    content << "BT\n";
    content << "/F1 20 Tf\n";
    content << "60 755 Td\n(" << EscapePDFText("MEDICAL INVOICE") << ") Tj\n";
    content << "/F1 10 Tf\n";
    content << "380 0 Td\n(" << EscapePDFText(invoiceId) << ") Tj\n";
    content << "ET\n";
    
    // Patient info section
    content << "BT\n";
    content << "/F1 11 Tf\n";
    content << "60 705 Td\n(" << EscapePDFText("PATIENT INFORMATION") << ") Tj\n";
    content << "/F1 10 Tf\n";
    content << "0 -20 Td\n(" << EscapePDFText("   Name: " + patientName) << ") Tj\n";
    content << "0 -15 Td\n(" << EscapePDFText("   Date: " + issuedOn) << ") Tj\n";
    content << "ET\n";
    
    // Draw horizontal line
    content << "60 655 m 552 655 l S\n";
    
    // Services table header
    content << "q\n";
    content << "0.95 0.95 0.95 rg\n";
    content << "60 615 492 25 re f\n";
    content << "Q\n";
    content << "60 615 492 25 re S\n";
    
    content << "BT\n";
    content << "/F1 11 Tf\n";
    content << "70 622 Td\n(" << EscapePDFText("DESCRIPTION") << ") Tj\n";
    content << "370 0 Td\n(" << EscapePDFText("AMOUNT (Rs.)") << ") Tj\n";
    content << "ET\n";
    
    // Draw table rows
    int yPos = 615;
    
    // Consultation fee row
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Consultation Fee") << ") Tj\n";
    content << "415 0 Td\n(" << EscapePDFText(formatCurrency(CONSULTATION_FEE)) << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Medicines section
    if (medicineLineCount > 0 && medicineLinePrices[0] > 0)
    {
        content << "60 " << (yPos - 25) << " 492 25 re S\n";
        content << "BT\n";
        content << "/F1 11 Tf\n";
        content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("MEDICATIONS") << ") Tj\n";
        content << "ET\n";
        yPos -= 25;
        
        for (int i = 0; i < medicineLineCount; i++)
        {
            string name = medicineNames[i];
            if (name.size() > 40)
            {
                name = name.substr(0, 37) + "...";
            }
            
            content << "60 " << (yPos - 20) << " 492 20 re S\n";
            content << "BT\n";
            content << "/F1 10 Tf\n";
            content << "80 " << (yPos - 13) << " Td\n(" << EscapePDFText("   - " + name) << ") Tj\n";
            content << "385 0 Td\n(" << EscapePDFText(formatCurrency(medicineLinePrices[i])) << ") Tj\n";
            content << "ET\n";
            yPos -= 20;
        }
    }
    
    // Subtotal line
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Medicine Charges") << ") Tj\n";
    content << "415 0 Td\n(" << EscapePDFText(formatCurrency(medicineTotal)) << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Draw thick line before total
    content << "2 w\n";
    content << "60 " << yPos << " m 552 " << yPos << " l S\n";
    content << "0.5 w\n";
    yPos -= 5;
    
    // Total section with highlight
    content << "q\n";
    content << "0.95 0.95 1 rg\n";
    content << "60 " << (yPos - 30) << " 492 30 re f\n";
    content << "Q\n";
    content << "60 " << (yPos - 30) << " 492 30 re S\n";
    
    content << "BT\n";
    content << "/F1 14 Tf\n";
    content << "70 " << (yPos - 20) << " Td\n(" << EscapePDFText("TOTAL AMOUNT DUE") << ") Tj\n";
    content << "320 0 Td\n(" << EscapePDFText(formatCurrency(total)) << ") Tj\n";
    content << "ET\n";
    yPos -= 30;
    
    // Footer section
    yPos -= 20;
    content << "BT\n";
    content << "/F1 9 Tf\n";
    content << "0.4 0.4 0.4 rg\n";
    content << "60 " << (yPos - 5) << " Td\n(" << EscapePDFText("Payment Terms: Due upon receipt") << ") Tj\n";
    content << "0 -15 Td\n(" << EscapePDFText("Thank you for trusting us with your healthcare needs.") << ") Tj\n";
    content << "0 -12 Td\n(" << EscapePDFText("Please retain this invoice for your records.") << ") Tj\n";
    content << "ET\n";

    string contentStr = content.str();

    ostringstream pdf;
    pdf << "%PDF-1.4\n";
    long long offsets[6] = {};

    offsets[1] = static_cast<long long>(pdf.tellp());
    pdf << "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n";
    offsets[2] = static_cast<long long>(pdf.tellp());
    pdf << "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n";
    offsets[3] = static_cast<long long>(pdf.tellp());
    pdf << "3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >> endobj\n";
    offsets[4] = static_cast<long long>(pdf.tellp());
    pdf << "4 0 obj << /Length " << contentStr.size() << " >> stream\n" << contentStr << "\nendstream\nendobj\n";
    offsets[5] = static_cast<long long>(pdf.tellp());
    pdf << "5 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj\n";

    long long xrefPos = static_cast<long long>(pdf.tellp());
    pdf << "xref\n0 6\n0000000000 65535 f \n";
    pdf << setfill('0');
    for (int i = 1; i <= 5; i++)
    {
        pdf << setw(10) << offsets[i] << " 00000 n \n";
    }
    pdf << setfill(' ');
    pdf << "trailer << /Size 6 /Root 1 0 R >>\n";
    pdf << "startxref\n" << xrefPos << "\n%%EOF";

    string baseName = SanitizeFileName(patientName);
    if (baseName.empty())
    {
        baseName = "patient";
    }
    string fileName = baseName + ".pdf";
    ofstream out(fileName, ios::binary);
    if (!out.is_open())
    {
        cerr << "Unable to create bill file." << endl;
        return false;
    }

    out << pdf.str();
    cout << "Bill saved as " << fileName << endl;
    return true;
}

// Generate Revenue Report PDF
bool GenerateRevenueReportPDF(const StrArray &appointmentStatus, const StrArray &appointmentDate,
                              const StrArray &appointmentEmergency, int appointmentCount)
{
    int completedAppointments = 0;
    int emergencyAppointments = 0;
    int regularAppointments = 0;
    int consultationRevenue = 0;
    int emergencyRevenue = 0;
    int medicineRevenue = 0; // Estimated
    
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentStatus[i] == "Completed" || appointmentStatus[i] == "completed" || 
            appointmentStatus[i] == "Emergency-Completed")
        {
            completedAppointments++;
            bool isEmergency = (appointmentEmergency[i] == "Yes");
            if (isEmergency)
            {
                emergencyAppointments++;
                emergencyRevenue += EMERGENCY_CONSULTATION_FEE;
                consultationRevenue += EMERGENCY_CONSULTATION_FEE;
            }
            else
            {
                regularAppointments++;
                consultationRevenue += CONSULTATION_FEE;
            }
        }
    }
    
    // Estimated medicine revenue (33% of consultation revenue)
    medicineRevenue = consultationRevenue / 3;
    int totalRevenue = consultationRevenue + medicineRevenue;
    
    auto formatCurrency = [](int value) {
        return string("Rs.") + to_string(value);
    };
    
    time_t now = time(nullptr);
    tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localTime = *localtime(&now);
#endif
    ostringstream dateStream;
    dateStream << put_time(&localTime, "%Y-%m-%d %H:%M");
    string generatedOn = dateStream.str();
    
    ostringstream reportStream;
    reportStream << "REV-" << put_time(&localTime, "%Y%m%d%H%M");
    string reportId = reportStream.str();
    
    ostringstream content;
    
    // Draw header box
    content << "q\n";
    content << "0.9 0.95 0.9 rg\n";
    content << "50 730 512 50 re f\n";
    content << "Q\n";
    content << "0.5 w\n";
    content << "50 730 512 50 re S\n";
    
    // Header text
    content << "BT\n";
    content << "/F1 20 Tf\n";
    content << "60 755 Td\n(" << EscapePDFText("MONTHLY REVENUE REPORT") << ") Tj\n";
    content << "/F1 10 Tf\n";
    content << "360 0 Td\n(" << EscapePDFText(reportId) << ") Tj\n";
    content << "ET\n";
    
    // Report info section
    content << "BT\n";
    content << "/F1 11 Tf\n";
    content << "60 705 Td\n(" << EscapePDFText("REPORT DETAILS") << ") Tj\n";
    content << "/F1 10 Tf\n";
    content << "0 -20 Td\n(" << EscapePDFText("   Generated: " + generatedOn) << ") Tj\n";
    content << "0 -15 Td\n(" << EscapePDFText("   Period: All Time") << ") Tj\n";
    content << "ET\n";
    
    // Draw horizontal line
    content << "60 655 m 552 655 l S\n";
    
    // Statistics section
    content << "BT\n";
    content << "/F1 12 Tf\n";
    content << "60 630 Td\n(" << EscapePDFText("APPOINTMENT STATISTICS") << ") Tj\n";
    content << "ET\n";
    
    int yPos = 605;
    
    // Stats box
    content << "q\n";
    content << "0.98 0.98 0.98 rg\n";
    content << "60 " << (yPos - 75) << " 492 75 re f\n";
    content << "Q\n";
    content << "60 " << (yPos - 75) << " 492 75 re S\n";
    
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Total Completed Appointments:") << ") Tj\n";
    content << "300 0 Td\n(" << EscapePDFText(to_string(completedAppointments)) << ") Tj\n";
    content << "-300 -20 Td\n(" << EscapePDFText("   Regular Appointments:") << ") Tj\n";
    content << "300 0 Td\n(" << EscapePDFText(to_string(regularAppointments)) << ") Tj\n";
    content << "-300 -20 Td\n(" << EscapePDFText("   Emergency Appointments:") << ") Tj\n";
    content << "300 0 Td\n(" << EscapePDFText(to_string(emergencyAppointments)) << ") Tj\n";
    content << "ET\n";
    
    yPos -= 95;
    
    // Revenue breakdown section
    content << "BT\n";
    content << "/F1 12 Tf\n";
    content << "60 " << yPos << " Td\n(" << EscapePDFText("REVENUE BREAKDOWN") << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Revenue table header
    content << "q\n";
    content << "0.95 0.95 0.95 rg\n";
    content << "60 " << (yPos - 25) << " 492 25 re f\n";
    content << "Q\n";
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    
    content << "BT\n";
    content << "/F1 11 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("CATEGORY") << ") Tj\n";
    content << "350 0 Td\n(" << EscapePDFText("AMOUNT (Rs.)") << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Regular consultation row
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Regular Consultations (" + to_string(regularAppointments) + ")") << ") Tj\n";
    content << "365 0 Td\n(" << EscapePDFText(formatCurrency(regularAppointments * CONSULTATION_FEE)) << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Emergency consultation row
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Emergency Consultations (" + to_string(emergencyAppointments) + ")") << ") Tj\n";
    content << "365 0 Td\n(" << EscapePDFText(formatCurrency(emergencyRevenue)) << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Medicine sales row
    content << "60 " << (yPos - 25) << " 492 25 re S\n";
    content << "BT\n";
    content << "/F1 10 Tf\n";
    content << "70 " << (yPos - 18) << " Td\n(" << EscapePDFText("   Medicine Sales (estimated)") << ") Tj\n";
    content << "365 0 Td\n(" << EscapePDFText(formatCurrency(medicineRevenue)) << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Draw thick line before total
    content << "2 w\n";
    content << "60 " << yPos << " m 552 " << yPos << " l S\n";
    content << "0.5 w\n";
    yPos -= 5;
    
    // Total section with highlight
    content << "q\n";
    content << "0.9 1 0.9 rg\n";
    content << "60 " << (yPos - 35) << " 492 35 re f\n";
    content << "Q\n";
    content << "60 " << (yPos - 35) << " 492 35 re S\n";
    
    content << "BT\n";
    content << "/F1 16 Tf\n";
    content << "70 " << (yPos - 23) << " Td\n(" << EscapePDFText("TOTAL REVENUE") << ") Tj\n";
    content << "280 0 Td\n(" << EscapePDFText(formatCurrency(totalRevenue)) << ") Tj\n";
    content << "ET\n";
    yPos -= 35;
    
    // Revenue distribution visualization
    yPos -= 30;
    content << "BT\n";
    content << "/F1 11 Tf\n";
    content << "60 " << yPos << " Td\n(" << EscapePDFText("REVENUE DISTRIBUTION") << ") Tj\n";
    content << "ET\n";
    yPos -= 20;
    
    // Simple bar chart representation
    int consultationPercent = (totalRevenue > 0) ? (consultationRevenue * 100 / totalRevenue) : 0;
    int medicinePercent = 100 - consultationPercent;
    
    // Consultation bar
    content << "q\n";
    content << "0.3 0.6 0.9 rg\n";
    int consultationBarWidth = (consultationPercent * 350) / 100;
    content << "70 " << (yPos - 20) << " " << consultationBarWidth << " 15 re f\n";
    content << "Q\n";
    
    content << "BT\n";
    content << "/F1 9 Tf\n";
    content << "75 " << (yPos - 14) << " Td\n(" << EscapePDFText("Consultations " + to_string(consultationPercent) + "%") << ") Tj\n";
    content << "ET\n";
    yPos -= 25;
    
    // Medicine bar
    content << "q\n";
    content << "0.3 0.8 0.5 rg\n";
    int medicineBarWidth = (medicinePercent * 350) / 100;
    content << "70 " << (yPos - 20) << " " << medicineBarWidth << " 15 re f\n";
    content << "Q\n";
    
    content << "BT\n";
    content << "/F1 9 Tf\n";
    content << "75 " << (yPos - 14) << " Td\n(" << EscapePDFText("Medicines " + to_string(medicinePercent) + "%") << ") Tj\n";
    content << "ET\n";
    yPos -= 40;
    
    // Footer section
    content << "BT\n";
    content << "/F1 9 Tf\n";
    content << "0.4 0.4 0.4 rg\n";
    content << "60 " << (yPos - 5) << " Td\n(" << EscapePDFText("This report is generated automatically by the Hospital Management System.") << ") Tj\n";
    content << "0 -15 Td\n(" << EscapePDFText("For detailed analysis, please consult the finance department.") << ") Tj\n";
    content << "0 -12 Td\n(" << EscapePDFText("Confidential - For internal use only.") << ") Tj\n";
    content << "ET\n";
    
    string contentStr = content.str();
    
    ostringstream pdf;
    pdf << "%PDF-1.4\n";
    long long offsets[6] = {};
    
    offsets[1] = static_cast<long long>(pdf.tellp());
    pdf << "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n";
    offsets[2] = static_cast<long long>(pdf.tellp());
    pdf << "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n";
    offsets[3] = static_cast<long long>(pdf.tellp());
    pdf << "3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >> endobj\n";
    offsets[4] = static_cast<long long>(pdf.tellp());
    pdf << "4 0 obj << /Length " << contentStr.size() << " >> stream\n" << contentStr << "\nendstream\nendobj\n";
    offsets[5] = static_cast<long long>(pdf.tellp());
    pdf << "5 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj\n";
    
    long long xrefPos = static_cast<long long>(pdf.tellp());
    pdf << "xref\n0 6\n0000000000 65535 f \n";
    pdf << setfill('0');
    for (int i = 1; i <= 5; i++)
    {
        pdf << setw(10) << offsets[i] << " 00000 n \n";
    }
    pdf << setfill(' ');
    pdf << "trailer << /Size 6 /Root 1 0 R >>\n";
    pdf << "startxref\n" << xrefPos << "\n%%EOF";
    
    ostringstream fileNameStream;
    fileNameStream << "Revenue_Report_" << put_time(&localTime, "%Y%m%d_%H%M%S") << ".pdf";
    string fileName = fileNameStream.str();
    
    ofstream out(fileName, ios::binary);
    if (!out.is_open())
    {
        cerr << "Unable to create revenue report file." << endl;
        return false;
    }
    
    out << pdf.str();
    cout << "Revenue report saved as " << fileName << endl;
    return true;
}

// ============== UI HELPER FUNCTIONS ==============

// Setup console for UTF-8 and colors
void setupConsole()
{
    // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    // Enable ANSI escape sequences for better color support
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, mode);
}

// Get left padding to center a box of given width
int getCenterPadding(int boxWidth)
{
    const int SCREEN_WIDTH = 120; // Standard console width
    int padding = (SCREEN_WIDTH - boxWidth) / 2;
    return padding > 0 ? padding : 0;
}

void useRedColour()
{
    // Used to set the console text colour to red.
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 12);
}

// Changes the text colors
void useWhiteColour()
{
    // Used to set the console text colour to white.
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 15);
}

void useGreenColour()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 10);
}

void useCyanColour()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 11);
}

void useYellowColour()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 14);
}

void useMagentaColour()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 13);
}

// Safe integer input with validation
int SafeIntInput(const string &message, int minValue = INT_MIN, int maxValue = INT_MAX)
{
    int value;
    while (true)
    {
        cout << message;
        if (cin >> value)
        {
            // Check if the value is within the specified range
            if (value >= minValue && value <= maxValue)
            {
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear the buffer
                return value;
            }
            else
            {
                useRedColour();
                cout << "❌ Invalid input! Please enter a number between " << minValue << " and " << maxValue << "." << endl;
                useWhiteColour();
            }
        }
        else
        {
            // Handle non-integer input
            useRedColour();
            cout << "❌ Invalid input! Please enter a valid integer." << endl;
            useWhiteColour();
            cin.clear(); // Clear the error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
        }
    }
}

// Safe character input with validation
char SafeCharInput(const string &message, const string &validChars = "")
{
    char value;
    string input;
    while (true)
    {
        cout << message;
        if (cin >> input)
        {
            if (input.length() == 1)
            {
                value = input[0];
                if (validChars.empty() || validChars.find(toupper(value)) != string::npos)
                {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    return value;
                }
                else
                {
                    useRedColour();
                    cout << "❌ Invalid input! Please enter one of: " << validChars << endl;
                    useWhiteColour();
                }
            }
            else
            {
                useRedColour();
                cout << "❌ Invalid input! Please enter a single character." << endl;
                useWhiteColour();
            }
        }
        else
        {
            useRedColour();
            cout << "❌ Invalid input! Please try again." << endl;
            useWhiteColour();
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// Display success message
void showSuccess(const string &message)
{
    useGreenColour();
    cout << "\n✓ " << message << endl;
    useWhiteColour();
}

// Display error message
void showError(const string &message)
{
    useRedColour();
    cout << "\n✗ " << message << endl;
    useWhiteColour();
}

// Display info message
void showInfo(const string &message)
{
    useCyanColour();
    cout << "\nℹ " << message << endl;
    useWhiteColour();
}

// Draw table header
void drawTableHeader(const string &col1, const string &col2, const string &col3, const string &col4)
{
    useCyanColour();
    cout << "\n╔═══════════════════╦═══════════════════╦═══════════════════╦═══════════════════╗" << endl;
    cout << "║ " << left << setw(17) << col1 << " ║ " << setw(17) << col2 << " ║ " << setw(17) << col3 << " ║ " << setw(17) << col4 << " ║" << endl;
    cout << "╠═══════════════════╬═══════════════════╬═══════════════════╬═══════════════════╣" << endl;
    useWhiteColour();
}

// Draw table row
void drawTableRow(const string &col1, const string &col2, const string &col3, const string &col4)
{
    useCyanColour();
    cout << "║ ";
    useWhiteColour();
    cout << left << setw(17) << col1.substr(0, 17);
    useCyanColour();
    cout << " ║ ";
    useWhiteColour();
    cout << setw(17) << col2.substr(0, 17);
    useCyanColour();
    cout << " ║ ";
    useWhiteColour();
    cout << setw(17) << col3.substr(0, 17);
    useCyanColour();
    cout << " ║ ";
    useWhiteColour();
    cout << setw(17) << col4.substr(0, 17);
    useCyanColour();
    cout << " ║" << endl;
}

// Draw table footer
void drawTableFooter()
{
    useCyanColour();
    cout << "╚═══════════════════╩═══════════════════╩═══════════════════╩═══════════════════╝" << endl;
    useWhiteColour();
}

// Display menu box
void displayMenuBox(const string &title, const string options[], int optionCount, bool hasExit = true)
{
    const int BOX_WIDTH = 70;
    useCyanColour();
    cout << "\n╔";
    for (int i = 0; i < BOX_WIDTH; i++) cout << "═";
    cout << "╗" << endl;
    
    // Title
    useMagentaColour();
    useCyanColour();
    cout << "║";
    useMagentaColour();
    int titlePadding = (BOX_WIDTH - static_cast<int>(title.length())) / 2;
    cout << string(titlePadding, ' ') << title << string(BOX_WIDTH - titlePadding - title.length(), ' ');
    useCyanColour();
    cout << "║" << endl;
    
    cout << "╠";
    for (int i = 0; i < BOX_WIDTH; i++) cout << "═";
    cout << "╣" << endl;
    
    // Options
    for (int i = 0; i < optionCount; i++)
    {
        useCyanColour();
        cout << "║";
        
        if (hasExit && i == optionCount - 1)
            useRedColour();
        else
            useGreenColour();
        
        string optionText = "  [" + to_string(i + 1) + "] ► " + options[i];
        int contentWidth = static_cast<int>(optionText.length());
        cout << left << optionText << string(BOX_WIDTH - contentWidth, ' ');
        useCyanColour();
        cout << "║" << endl;
    }
    
    cout << "╚";
    for (int i = 0; i < BOX_WIDTH; i++) cout << "═";
    cout << "╝" << endl;
    
    useYellowColour();
    cout << "\nEnter your choice: ";
    useWhiteColour();
}

// Animated text with delay
void printAnimated(const string &text, int delayMs = 30)
{
    for (char c : text)
    {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(delayMs));
    }
}

// Loading animation
void showLoading(const string &message, int durationMs = 1000)
{
    cout << message;
    const char spinner[] = {'|', '/', '-', '\\'};
    int iterations = durationMs / 100;
    for (int i = 0; i < iterations; i++)
    {
        cout << " " << spinner[i % 4] << "\r" << message << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    cout << " Done!" << endl;
}

// Draw a box
void drawBox(int width)
{
    useCyanColour();
    cout << "╔"; // Top-left corner
    for (int i = 0; i < width - 2; i++)
        cout << "═"; // Horizontal line
    cout << "╗" << endl; // Top-right corner
    useWhiteColour();
}

void drawBoxBottom(int width)
{
    useCyanColour();
    cout << "╚"; // Bottom-left corner
    for (int i = 0; i < width - 2; i++)
        cout << "═"; // Horizontal line
    cout << "╝" << endl; // Bottom-right corner
    useWhiteColour();
}

void drawBoxSides(const string &text, int width)
{
    useCyanColour();
    cout << "║"; // Vertical line
    useWhiteColour();
    int padding = (width - static_cast<int>(text.length()) - 2) / 2;
    for (int i = 0; i < padding; i++)
        cout << " ";
    cout << text;
    for (int i = 0; i < width - static_cast<int>(text.length()) - padding - 2; i++)
        cout << " ";
    useCyanColour();
    cout << "║" << endl; // Vertical line
    useWhiteColour();
}

// Used to print the header of the application.
void printHeader()
{
    cout << endl;
    int width = 100;
    drawBox(width);
    
    // Empty line
    drawBoxSides("", width);
    
    // Title line
    useMagentaColour();
    string title = "HOSPITAL MANAGEMENT SYSTEM";
    int titlePadding = (width - static_cast<int>(title.length()) - 2) / 2;
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << string(titlePadding, ' ');
    useMagentaColour();
    for (char c : title)
    {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    useWhiteColour();
    cout << string(width - titlePadding - title.length() - 2, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Empty line
    useWhiteColour();
    drawBoxSides("", width);
    
    // Subtitle line
    useGreenColour();
    string subtitle = "Your Health, Our Priority";
    int subtitlePadding = (width - static_cast<int>(subtitle.length()) - 2) / 2;
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << string(subtitlePadding, ' ');
    useGreenColour();
    for (char c : subtitle)
    {
        cout << c << flush;
        this_thread::sleep_for(chrono::milliseconds(40));
    }
    useWhiteColour();
    cout << string(width - subtitlePadding - subtitle.length() - 2, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Empty line
    useWhiteColour();
    drawBoxSides("", width);
    
    drawBoxBottom(width);
    useWhiteColour();
    cout << endl;
}

// ============== END OF UI HELPER FUNCTIONS ==============

// Forward declarations
void clearScreen();

// ============== NEW FEATURE FUNCTIONS ==============

// Get current date in YYYY-MM-DD format
string getCurrentDate()
{
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buffer[11];
    sprintf(buffer, "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
    return string(buffer);
}

// Calculate days between two dates (simple implementation)
int daysBetweenDates(const string &date1, const string &date2)
{
    // Simple calculation - assumes format YYYY-MM-DD
    int year1, month1, day1, year2, month2, day2;
    sscanf(date1.c_str(), "%d-%d-%d", &year1, &month1, &day1);
    sscanf(date2.c_str(), "%d-%d-%d", &year2, &month2, &day2);
    
    int days1 = year1 * 365 + month1 * 30 + day1;
    int days2 = year2 * 365 + month2 * 30 + day2;
    return days2 - days1;
}

// Display table header
void displayTableHeader(const string &col1, const string &col2, const string &col3, const string &col4, int width)
{
    useCyanColour();
    cout << "╔";
    for (int i = 0; i < width; i++) cout << "═";
    cout << "╗" << endl;
    
    cout << "║ " << left << setw(12) << col1 << " │ " << setw(15) << col2 << " │ " 
         << setw(12) << col3 << " │ " << setw(10) << col4 << " ║" << endl;
    
    cout << "╠";
    for (int i = 0; i < width; i++) cout << "═";
    cout << "╣" << endl;
    useWhiteColour();
}

// Display table row
void displayTableRow(const string &col1, const string &col2, const string &col3, const string &col4)
{
    useCyanColour();
    cout << "║ ";
    useWhiteColour();
    cout << left << setw(12) << col1;
    useCyanColour();
    cout << " │ ";
    useWhiteColour();
    cout << setw(15) << col2;
    useCyanColour();
    cout << " │ ";
    useWhiteColour();
    cout << setw(12) << col3;
    useCyanColour();
    cout << " │ ";
    useWhiteColour();
    cout << setw(10) << col4;
    useCyanColour();
    cout << " ║" << endl;
    useWhiteColour();
}

// Close table
void closeTable(int width)
{
    useCyanColour();
    cout << "╚";
    for (int i = 0; i < width; i++) cout << "═";
    cout << "╝" << endl;
    useWhiteColour();
}

// Display progress bar
void displayProgressBar(const string &label, int current, int total, int barWidth = 20)
{
    cout << label << " [";
    useGreenColour();
    int filled = (current * barWidth) / total;
    for (int i = 0; i < barWidth; i++)
    {
        if (i < filled) cout << "█";
        else
        {
            useCyanColour();
            cout << "░";
        }
    }
    useWhiteColour();
    cout << "] " << current << "/" << total;
    if (total > 0)
    {
        int percent = (current * 100) / total;
        cout << " (" << percent << "%)";
    }
    cout << endl;
}

// Patient Appointment History Dashboard
void showPatientHistory(const string &patientName, const StrArray &appointmentPatient, const StrArray &appointmentDoctor,
                        const StrArray &appointmentDate, const StrArray &appointmentTime, const StrArray &appointmentStatus,
                        const StrArray &appointmentEmergency, int appointmentCount)
{
    clearScreen();
    printHeader();
    
    useGreenColour();
    cout << "\n════════════════════════════════════════════════════════════════" << endl;
    cout << "           📋 YOUR APPOINTMENT HISTORY" << endl;
    cout << "════════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
    
    int patientAppointments = 0;
    int totalSpent = 0;
    string mostVisitedDoctor;
    int maxVisits = 0;
    
    // Count doctor visits
    string doctorNames[UserSize];
    int doctorVisits[UserSize] = {0};
    int uniqueDoctors = 0;
    
    // Display table
    displayTableHeader("Date", "Doctor", "Status", "Amount", 68);
    
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentPatient[i] == patientName)
        {
            patientAppointments++;
            string status = appointmentStatus[i];
            bool isEmergency = (appointmentEmergency[i] == "Yes");
            int amount = 0;
            if (status == "Completed" || status == "completed" || status == "Emergency-Completed")
            {
                amount = isEmergency ? EMERGENCY_CONSULTATION_FEE : CONSULTATION_FEE;
            }
            totalSpent += amount;
            
            // Count doctor visits
            bool found = false;
            for (int j = 0; j < uniqueDoctors; j++)
            {
                if (doctorNames[j] == appointmentDoctor[i])
                {
                    doctorVisits[j]++;
                    found = true;
                    if (doctorVisits[j] > maxVisits)
                    {
                        maxVisits = doctorVisits[j];
                        mostVisitedDoctor = appointmentDoctor[i];
                    }
                    break;
                }
            }
            if (!found && uniqueDoctors < UserSize)
            {
                doctorNames[uniqueDoctors] = appointmentDoctor[i];
                doctorVisits[uniqueDoctors] = 1;
                if (1 > maxVisits)
                {
                    maxVisits = 1;
                    mostVisitedDoctor = appointmentDoctor[i];
                }
                uniqueDoctors++;
            }
            
            string amountStr = (amount > 0) ? ("Rs." + to_string(amount)) : "-";
            string doctorDisplay = appointmentDoctor[i];
            if (isEmergency) {
                doctorDisplay = "🚨 " + doctorDisplay;
            }
            displayTableRow(appointmentDate[i], doctorDisplay, status, amountStr);
        }
    }
    
    closeTable(68);
    
    // Display statistics
    useYellowColour();
    cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    useGreenColour();
    cout << "📊 Statistics: ";
    useWhiteColour();
    cout << "Total Visits: ";
    useMagentaColour();
    cout << patientAppointments;
    useWhiteColour();
    cout << "  |  Most Visited: ";
    useMagentaColour();
    cout << (mostVisitedDoctor.empty() ? "N/A" : mostVisitedDoctor);
    useWhiteColour();
    cout << "  |  Total Spent: ";
    useGreenColour();
    cout << "Rs." << totalSpent << endl;
    useYellowColour();
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    useWhiteColour();
    
    cout << "\nPress any key to continue...";
    _getch();
}

// Appointment Reminder System
void showUpcomingAppointments(const string &patientName, const StrArray &appointmentPatient,
                               const StrArray &appointmentDoctor, const StrArray &appointmentDate,
                               const StrArray &appointmentTime, const StrArray &appointmentStatus,
                               int appointmentCount)
{
    string today = getCurrentDate();
    bool hasReminders = false;
    
    useYellowColour();
    cout << "\n🔔 UPCOMING APPOINTMENTS" << endl;
    useCyanColour();
    cout << "════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
    
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentPatient[i] == patientName && appointmentStatus[i] == "Pending")
        {
            int daysUntil = daysBetweenDates(today, appointmentDate[i]);
            if (daysUntil >= 0 && daysUntil <= 7)
            {
                hasReminders = true;
                if (daysUntil == 0)
                {
                    useRedColour();
                    cout << "🔴 TODAY: ";
                }
                else if (daysUntil == 1)
                {
                    useYellowColour();
                    cout << "⚠️  Tomorrow: ";
                }
                else
                {
                    useGreenColour();
                    cout << "📅 In " << daysUntil << " days: ";
                }
                useWhiteColour();
                cout << "Appointment with " << appointmentDoctor[i] 
                     << " at " << appointmentTime[i] << endl;
            }
        }
    }
    
    if (!hasReminders)
    {
        useGreenColour();
        cout << "✓ No upcoming appointments in the next 7 days." << endl;
    }
    
    useCyanColour();
    cout << "════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
}

// Doctor Performance Dashboard
void showDoctorPerformance(const string &doctorName, const StrArray &appointmentDoctor,
                           const StrArray &appointmentDate, const StrArray &appointmentStatus,
                           int appointmentCount)
{
    clearScreen();
    printHeader();
    
    useMagentaColour();
    cout << "\n════════════════════════════════════════════════════════════" << endl;
    cout << "        👨‍⚕️ DOCTOR PERFORMANCE DASHBOARD" << endl;
    cout << "════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
    
    string today = getCurrentDate();
    int patientsToday = 0;
    int patientsThisWeek = 0;
    int completedAppointments = 0;
    int totalAppointments = 0;
    
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentDoctor[i] == doctorName)
        {
            totalAppointments++;
            if (appointmentStatus[i] == "Completed" || appointmentStatus[i] == "completed")
            {
                completedAppointments++;
            }
            
            int daysAgo = daysBetweenDates(appointmentDate[i], today);
            if (daysAgo == 0) patientsToday++;
            if (daysAgo >= 0 && daysAgo <= 7) patientsThisWeek++;
        }
    }
    
    int workloadPercent = (totalAppointments > 0) ? (patientsThisWeek * 10) : 0;
    if (workloadPercent > 100) workloadPercent = 100;
    
    useGreenColour();
    cout << "\nDoctor: ";
    useMagentaColour();
    cout << doctorName << endl;
    useWhiteColour();
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    
    useYellowColour();
    cout << "\n📊 Performance Metrics:" << endl;
    useWhiteColour();
    cout << "  • Patients Today:        " << patientsToday << endl;
    cout << "  • Patients This Week:    " << patientsThisWeek << endl;
    cout << "  • Total Appointments:    " << totalAppointments << endl;
    cout << "  • Completed:             " << completedAppointments << endl;
    
    cout << "\n";
    displayProgressBar("  Weekly Workload", patientsThisWeek, 50);
    
    useWhiteColour();
    cout << "\nPress any key to continue...";
    _getch();
}

// Medicine Low Stock Alert
void checkLowStockMedicines(const StrArray &Medicines, const IntArray &MedicineStock,
                            const IntArray &MedicineReorderLevel, int MedicineCount)
{
    int lowStockCount = 0;
    
    for (int i = 0; i < MedicineCount; i++)
    {
        if (MedicineStock[i] < MedicineReorderLevel[i])
        {
            lowStockCount++;
        }
    }
    
    if (lowStockCount > 0)
    {
        useRedColour();
        cout << "\n⚠️  ALERT: " << lowStockCount << " medicine(s) need reordering!" << endl;
        useWhiteColour();
    }
}

// Display Low Stock Medicines
void displayLowStockReport(const StrArray &Medicines, const IntArray &MedicineStock,
                           const IntArray &MedicineReorderLevel, int MedicineCount)
{
    clearScreen();
    printHeader();
    
    useRedColour();
    cout << "\n════════════════════════════════════════════════════════════" << endl;
    cout << "        ⚠️  LOW STOCK MEDICINES REPORT" << endl;
    cout << "════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
    
    bool foundLowStock = false;
    
    useCyanColour();
    cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║ Medicine Name       │ Current Stock │ Reorder Level  ║" << endl;
    cout << "╠════════════════════════════════════════════════════════╣" << endl;
    useWhiteColour();
    
    for (int i = 0; i < MedicineCount; i++)
    {
        if (MedicineStock[i] < MedicineReorderLevel[i])
        {
            foundLowStock = true;
            useCyanColour();
            cout << "║ ";
            useRedColour();
            cout << left << setw(19) << Medicines[i];
            useCyanColour();
            cout << " │ ";
            useWhiteColour();
            cout << setw(13) << MedicineStock[i];
            useCyanColour();
            cout << " │ ";
            useWhiteColour();
            cout << setw(14) << MedicineReorderLevel[i];
            useCyanColour();
            cout << " ║" << endl;
            useWhiteColour();
        }
    }
    
    if (!foundLowStock)
    {
        useGreenColour();
        cout << "║           ✓ All medicines are adequately stocked          ║" << endl;
    }
    
    useCyanColour();
    cout << "╚════════════════════════════════════════════════════════╝" << endl;
    useWhiteColour();
    
    cout << "\nPress any key to continue...";
    _getch();
}

// Quick Stats Dashboard
void displayQuickStats(int patientCount, int appointmentCount, int doctorCount, int medicineCount,
                       const StrArray &appointmentStatus, int totalAppointments)
{
    int appointmentsToday = 0;
    string today = getCurrentDate();
    
    useGreenColour();
    cout << "\n╔═══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║                    📊 QUICK STATISTICS                        ║" << endl;
    cout << "╚═══════════════════════════════════════════════════════════════╝" << endl;
    useWhiteColour();
    
    useCyanColour();
    cout << "  👥 Total Patients:      ";
    useMagentaColour();
    cout << patientCount << endl;
    
    useCyanColour();
    cout << "  👨‍⚕️ Doctors Available:   ";
    useMagentaColour();
    cout << doctorCount << endl;
    
    useCyanColour();
    cout << "  📅 Total Appointments:  ";
    useMagentaColour();
    cout << appointmentCount << endl;
    
    useCyanColour();
    cout << "  💊 Medicines in Stock:  ";
    useMagentaColour();
    cout << medicineCount << endl;
    useWhiteColour();
    
    cout << endl;
}

// Monthly Revenue Report
void generateRevenueReport(const StrArray &appointmentStatus, const StrArray &appointmentDate,
                           const StrArray &appointmentEmergency, int appointmentCount)
{
    clearScreen();
    printHeader();
    
    useGreenColour();
    cout << "\n════════════════════════════════════════════════════════════" << endl;
    cout << "          💰 MONTHLY REVENUE REPORT" << endl;
    cout << "════════════════════════════════════════════════════════════" << endl;
    useWhiteColour();
    
    int completedAppointments = 0;
    int emergencyAppointments = 0;
    int totalRevenue = 0;
    int emergencyRevenue = 0;
    
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentStatus[i] == "Completed" || appointmentStatus[i] == "completed" ||
            appointmentStatus[i] == "Emergency-Completed")
        {
            completedAppointments++;
            bool isEmergency = (appointmentEmergency[i] == "Yes");
            if (isEmergency)
            {
                emergencyAppointments++;
                emergencyRevenue += EMERGENCY_CONSULTATION_FEE;
                totalRevenue += EMERGENCY_CONSULTATION_FEE;
            }
            else
            {
                totalRevenue += CONSULTATION_FEE;
            }
        }
    }
    
    int medicineRevenue = totalRevenue / 3; // Estimated
    int consultationRevenue = totalRevenue;
    
    useYellowColour();
    cout << "\n📊 Revenue Breakdown:" << endl;
    useWhiteColour();
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    
    useCyanColour();
    cout << "\n  Consultation Fees:  ";
    useGreenColour();
    cout << "Rs. " << consultationRevenue << endl;
    
    if (emergencyRevenue > 0)
    {
        useCyanColour();
        cout << "    (Emergency: Rs. " << emergencyRevenue << ")" << endl;
    }
    
    useCyanColour();
    cout << "  Medicine Sales:     ";
    useGreenColour();
    cout << "Rs. " << medicineRevenue << " (estimated)" << endl;
    
    useCyanColour();
    cout << "  ────────────────────────────" << endl;
    cout << "  Total Revenue:      ";
    useMagentaColour();
    cout << "Rs. " << (consultationRevenue + medicineRevenue) << endl;
    useWhiteColour();
    
    cout << "\n";
    displayProgressBar("Consultation", consultationRevenue, consultationRevenue + medicineRevenue);
    displayProgressBar("Medicines", medicineRevenue, consultationRevenue + medicineRevenue);
    
    useWhiteColour();
    cout << "\n\n";
    useYellowColour();
    useWhiteColour();
    char choice = SafeCharInput("Would you like to generate a PDF report? (Y/N): ", "YN");
    
    if (choice == 'Y' || choice == 'y')
    {
        if (GenerateRevenueReportPDF(appointmentStatus, appointmentDate, appointmentEmergency, appointmentCount))
        {
            useGreenColour();
            cout << "\n✓ Revenue report PDF generated successfully!" << endl;
            useWhiteColour();
        }
        else
        {
            useRedColour();
            cout << "\n✗ Failed to generate PDF report." << endl;
            useWhiteColour();
        }
    }
    
    cout << "\nPress any key to continue...";
    _getch();
}

// ============== END OF NEW FEATURE FUNCTIONS ==============

void LoadPatientsFromFile(StrArray &PatientName, StrArray &PatientPassword, StrArray &PatientAge, int &Patientcount)
{
    ifstream file(PATIENTS_FILE);
    if (!file.is_open())
    {
        return;
    }

    Patientcount = 0;
    string line;
    while (getline(file, line) && Patientcount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[3];
        int fieldCount = SplitFields(line, fields, 3);
        if (fieldCount < 3)
        {
            continue;
        }

        PatientName[Patientcount] = fields[0];
        PatientPassword[Patientcount] = fields[1];
        PatientAge[Patientcount] = fields[2];
        Patientcount++;
    }
}

void SavePatientsToFile(const StrArray &PatientName, const StrArray &PatientPassword, const StrArray &PatientAge, int Patientcount)
{
    ofstream file(PATIENTS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write patients data." << endl;
        return;
    }

    for (int i = 0; i < Patientcount; i++)
    {
        file << PatientName[i] << FIELD_DELIMITER
             << PatientPassword[i] << FIELD_DELIMITER
             << PatientAge[i] << '\n';
    }
}

void LoadAppointmentsFromFile(StrArray &appointmentPatient,
                              StrArray &appointmentDoctor,
                              StrArray &appointmentDate,
                              StrArray &appointmentTime,
                              StrArray &appointmentStatus,
                              StrArray &appointmentEmergency,
                              StrArray &PrescriptionMed,
                              StrArray &MedicineTimes,
                              StrArray &PrescriptionDays,
                              int &appointmentCount)
{
    ifstream file(APPOINTMENTS_FILE);
    if (!file.is_open())
    {
        return;
    }

    appointmentCount = 0;
    string line;
    while (getline(file, line) && appointmentCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[9];
        int fieldCount = SplitFields(line, fields, 9);
        if (fieldCount < 3)
        {
            continue;
        }

        appointmentPatient[appointmentCount] = fields[0];
        appointmentDoctor[appointmentCount] = fields[1];
        appointmentDate[appointmentCount] = fields[2];

        if (fieldCount >= 9)
        {
            appointmentTime[appointmentCount] = fields[3];
            appointmentStatus[appointmentCount] = fields[4];
            appointmentEmergency[appointmentCount] = fields[5];
            PrescriptionMed[appointmentCount] = fields[6];
            MedicineTimes[appointmentCount] = fields[7];
            PrescriptionDays[appointmentCount] = fields[8];
        }
        else
        {
            appointmentTime[appointmentCount] = GetField(fields, fieldCount, 3);
            appointmentStatus[appointmentCount] = GetField(fields, fieldCount, 4);
            appointmentEmergency[appointmentCount] = GetField(fields, fieldCount, 5);
            if (appointmentEmergency[appointmentCount].empty()) {
                appointmentEmergency[appointmentCount] = "No";
            }
            PrescriptionMed[appointmentCount] = GetField(fields, fieldCount, 6);
            MedicineTimes[appointmentCount] = GetField(fields, fieldCount, 7);
            PrescriptionDays[appointmentCount] = GetField(fields, fieldCount, 8);
        }
        appointmentCount++;
    }
}

void SaveAppointmentsToFile(const StrArray &appointmentPatient,
                            const StrArray &appointmentDoctor,
                            const StrArray &appointmentDate,
                            const StrArray &appointmentTime,
                            const StrArray &appointmentStatus,
                            const StrArray &appointmentEmergency,
                            const StrArray &PrescriptionMed,
                            const StrArray &MedicineTimes,
                            const StrArray &PrescriptionDays,
                            int appointmentCount)
{
    ofstream file(APPOINTMENTS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write appointments data." << endl;
        return;
    }

    for (int i = 0; i < appointmentCount; i++)
    {
        file << appointmentPatient[i] << FIELD_DELIMITER
             << appointmentDoctor[i] << FIELD_DELIMITER
             << appointmentDate[i] << FIELD_DELIMITER
               << appointmentTime[i] << FIELD_DELIMITER
             << appointmentStatus[i] << FIELD_DELIMITER
             << appointmentEmergency[i] << FIELD_DELIMITER
             << PrescriptionMed[i] << FIELD_DELIMITER
             << MedicineTimes[i] << FIELD_DELIMITER
             << PrescriptionDays[i] << '\n';
    }
}

void LoadDoctorsFromFile(StrArray &Doctors,
                         StrArray &DoctorPassword,
                         StrArray &DoctorEmail,
                         StrArray &DoctorPhone,
                         StrArray &DoctorAddress,
                         StrArray &DoctorQualification,
                         StrArray &DoctorStartTime,
                         StrArray &DoctorEndTime,
                         StrArray &DocAvailableDays,
                         StrArray &DoctorSpecialization,
                         int &DoctorCount)
{
    ifstream file(DOCTORS_FILE);
    if (!file.is_open())
    {
        return;
    }

    DoctorCount = 0;
    string line;
    while (getline(file, line) && DoctorCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[10];
        int fieldCount = SplitFields(line, fields, 10);
        if (fieldCount < 6)
        {
            // Old format compatibility - load basic info
            if (fieldCount >= 5)
            {
                Doctors[DoctorCount] = fields[0];
                DoctorPassword[DoctorCount] = fields[1];
                DoctorEmail[DoctorCount] = "";
                DoctorPhone[DoctorCount] = "";
                DoctorAddress[DoctorCount] = "";
                DoctorQualification[DoctorCount] = "";
                DoctorStartTime[DoctorCount] = fields[2];
                DoctorEndTime[DoctorCount] = fields[3];
                DocAvailableDays[DoctorCount] = fields[4];
                DoctorSpecialization[DoctorCount] = (fieldCount >= 6 && !fields[5].empty()) ? fields[5] : "General Medicine";
                DoctorCount++;
            }
            continue;
        }

        Doctors[DoctorCount] = fields[0];
        DoctorPassword[DoctorCount] = fields[1];
        DoctorEmail[DoctorCount] = fields[2];
        DoctorPhone[DoctorCount] = fields[3];
        DoctorAddress[DoctorCount] = fields[4];
        DoctorQualification[DoctorCount] = fields[5];
        DoctorStartTime[DoctorCount] = (fieldCount >= 7) ? fields[6] : "09:00";
        DoctorEndTime[DoctorCount] = (fieldCount >= 8) ? fields[7] : "17:00";
        DocAvailableDays[DoctorCount] = (fieldCount >= 9) ? fields[8] : "Mon-Fri";
        DoctorSpecialization[DoctorCount] = (fieldCount >= 10 && !fields[9].empty()) ? fields[9] : "General Medicine";
        DoctorCount++;
    }
}

void SaveDoctorsToFile(const StrArray &Doctors,
                       const StrArray &DoctorPassword,
                       const StrArray &DoctorEmail,
                       const StrArray &DoctorPhone,
                       const StrArray &DoctorAddress,
                       const StrArray &DoctorQualification,
                       const StrArray &DoctorStartTime,
                       const StrArray &DoctorEndTime,
                       const StrArray &DocAvailableDays,
                       const StrArray &DoctorSpecialization,
                       int DoctorCount)
{
    ofstream file(DOCTORS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write doctors data." << endl;
        return;
    }

    for (int i = 0; i < DoctorCount; i++)
    {
           file << Doctors[i] << FIELD_DELIMITER
               << DoctorPassword[i] << FIELD_DELIMITER
               << DoctorEmail[i] << FIELD_DELIMITER
               << DoctorPhone[i] << FIELD_DELIMITER
               << DoctorAddress[i] << FIELD_DELIMITER
               << DoctorQualification[i] << FIELD_DELIMITER
               << DoctorStartTime[i] << FIELD_DELIMITER
               << DoctorEndTime[i] << FIELD_DELIMITER
               << DocAvailableDays[i] << FIELD_DELIMITER
               << DoctorSpecialization[i] << '\n';
    }
}

void LoadDoctorRequests(StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount)
{
    ifstream file(DOCTOR_REQUESTS_FILE);
    if (!file.is_open())
    {
        PendingDoctorRequestCount = 0;
        return;
    }

    PendingDoctorRequestCount = 0;
    string line;
    while (getline(file, line) && PendingDoctorRequestCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[2];
        int fieldCount = SplitFields(line, fields, 2);
        if (fieldCount < 2)
        {
            continue;
        }

        PendingDoctorNames[PendingDoctorRequestCount] = fields[0];
        PendingDoctorPasswords[PendingDoctorRequestCount] = fields[1];
        PendingDoctorRequestCount++;
    }
}

void SaveDoctorRequests(const StrArray &PendingDoctorNames, const StrArray &PendingDoctorPasswords, int PendingDoctorRequestCount)
{
    ofstream file(DOCTOR_REQUESTS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write doctor request data." << endl;
        return;
    }

    for (int i = 0; i < PendingDoctorRequestCount; i++)
    {
        file << PendingDoctorNames[i] << FIELD_DELIMITER
             << PendingDoctorPasswords[i] << '\n';
    }
}

void LoadReceptionistsFromFile(StrArray &Receptionist,
                               StrArray &ReceptionistPassword,
                               StrArray &ReceptionistEmail,
                               StrArray &ReceptionistPhone,
                               StrArray &ReceptionistAddress,
                               int &ReceptionistCount)
{
    ifstream file(RECEPTIONISTS_FILE);
    if (!file.is_open())
    {
        return;
    }

    ReceptionistCount = 0;
    string line;
    while (getline(file, line) && ReceptionistCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[5];
        int fieldCount = SplitFields(line, fields, 5);
        
        if (fieldCount == 1)
        {
            // Old format compatibility - name only
            Receptionist[ReceptionistCount] = fields[0];
            ReceptionistPassword[ReceptionistCount] = "";
            ReceptionistEmail[ReceptionistCount] = "";
            ReceptionistPhone[ReceptionistCount] = "";
            ReceptionistAddress[ReceptionistCount] = "";
        }
        else if (fieldCount >= 5)
        {
            // New format
            Receptionist[ReceptionistCount] = fields[0];
            ReceptionistPassword[ReceptionistCount] = fields[1];
            ReceptionistEmail[ReceptionistCount] = fields[2];
            ReceptionistPhone[ReceptionistCount] = fields[3];
            ReceptionistAddress[ReceptionistCount] = fields[4];
        }
        else
        {
            continue;
        }
        
        ReceptionistCount++;
    }
}

void SaveReceptionistsToFile(const StrArray &Receptionist,
                             const StrArray &ReceptionistPassword,
                             const StrArray &ReceptionistEmail,
                             const StrArray &ReceptionistPhone,
                             const StrArray &ReceptionistAddress,
                             int ReceptionistCount)
{
    ofstream file(RECEPTIONISTS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write receptionists data." << endl;
        return;
    }

    for (int i = 0; i < ReceptionistCount; i++)
    {
        file << Receptionist[i] << FIELD_DELIMITER
             << ReceptionistPassword[i] << FIELD_DELIMITER
             << ReceptionistEmail[i] << FIELD_DELIMITER
             << ReceptionistPhone[i] << FIELD_DELIMITER
             << ReceptionistAddress[i] << '\n';
    }
}

void LoadPharmacistsFromFile(StrArray &Pharmacist,
                             StrArray &PharmacistPassword,
                             StrArray &PharmacistEmail,
                             StrArray &PharmacistPhone,
                             StrArray &PharmacistAddress,
                             int &PharmacistCount)
{
    ifstream file(PHARMACISTS_FILE);
    if (!file.is_open())
    {
        return;
    }

    PharmacistCount = 0;
    string line;
    while (getline(file, line) && PharmacistCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[5];
        int fieldCount = SplitFields(line, fields, 5);
        
        if (fieldCount == 1)
        {
            // Old format compatibility - name only
            Pharmacist[PharmacistCount] = fields[0];
            PharmacistPassword[PharmacistCount] = "";
            PharmacistEmail[PharmacistCount] = "";
            PharmacistPhone[PharmacistCount] = "";
            PharmacistAddress[PharmacistCount] = "";
        }
        else if (fieldCount >= 5)
        {
            // New format
            Pharmacist[PharmacistCount] = fields[0];
            PharmacistPassword[PharmacistCount] = fields[1];
            PharmacistEmail[PharmacistCount] = fields[2];
            PharmacistPhone[PharmacistCount] = fields[3];
            PharmacistAddress[PharmacistCount] = fields[4];
        }
        else
        {
            continue;
        }
        
        PharmacistCount++;
    }
}

void SavePharmacistsToFile(const StrArray &Pharmacist,
                           const StrArray &PharmacistPassword,
                           const StrArray &PharmacistEmail,
                           const StrArray &PharmacistPhone,
                           const StrArray &PharmacistAddress,
                           int PharmacistCount)
{
    ofstream file(PHARMACISTS_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write pharmacists data." << endl;
        return;
    }

    for (int i = 0; i < PharmacistCount; i++)
    {
        file << Pharmacist[i] << FIELD_DELIMITER
             << PharmacistPassword[i] << FIELD_DELIMITER
             << PharmacistEmail[i] << FIELD_DELIMITER
             << PharmacistPhone[i] << FIELD_DELIMITER
             << PharmacistAddress[i] << '\n';
    }
}

void LoadMedicinesFromFile(StrArray &Medicines, StrArray &MedicinePrices,
                           IntArray &MedicineStock, IntArray &MedicineReorderLevel,
                           int &MedicineCount)
{
    ifstream file(MEDICINES_FILE);
    if (!file.is_open())
    {
        return;
    }

    MedicineCount = 0;
    string line;
    while (getline(file, line) && MedicineCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[4];
        int fieldCount = SplitFields(line, fields, 4);
        if (fieldCount < 2)
        {
            continue;
        }

        Medicines[MedicineCount] = fields[0];
        MedicinePrices[MedicineCount] = fields[1];
        if (fieldCount >= 3)
        {
            MedicineStock[MedicineCount] = SafeStringToInt(fields[2], 50);
        }
        else
        {
            MedicineStock[MedicineCount] = 50;
        }
        if (fieldCount >= 4)
        {
            MedicineReorderLevel[MedicineCount] = SafeStringToInt(fields[3], 20);
        }
        else
        {
            MedicineReorderLevel[MedicineCount] = 20;
        }
        MedicineCount++;
    }
}

void SaveMedicinesToFile(const StrArray &Medicines, const StrArray &MedicinePrices,
                         const IntArray &MedicineStock, const IntArray &MedicineReorderLevel,
                         int MedicineCount)
{
    ofstream file(MEDICINES_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write medicines data." << endl;
        return;
    }

    for (int i = 0; i < MedicineCount; i++)
    {
           file << Medicines[i] << FIELD_DELIMITER
               << MedicinePrices[i] << FIELD_DELIMITER
               << MedicineStock[i] << FIELD_DELIMITER
               << MedicineReorderLevel[i] << '\n';
    }
}

void LoadPatientHistory(StrArray &HistoryPatients, StrArray &HistoryDetails, int &HistoryCount)
{
    ifstream file(PATIENT_HISTORY_FILE);
    if (!file.is_open())
    {
        return;
    }

    HistoryCount = 0;
    string line;
    while (getline(file, line) && HistoryCount < UserSize)
    {
        if (line.empty())
        {
            continue;
        }

        string fields[2];
        int fieldCount = SplitFields(line, fields, 2);
        if (fieldCount < 2)
        {
            continue;
        }

        HistoryPatients[HistoryCount] = fields[0];
        HistoryDetails[HistoryCount] = fields[1];
        HistoryCount++;
    }
}

void SavePatientHistory(const StrArray &HistoryPatients, const StrArray &HistoryDetails, int HistoryCount)
{
    ofstream file(PATIENT_HISTORY_FILE);
    if (!file.is_open())
    {
        cerr << "Unable to write patient history data." << endl;
        return;
    }

    for (int i = 0; i < HistoryCount; i++)
    {
        file << HistoryPatients[i] << FIELD_DELIMITER
             << HistoryDetails[i] << '\n';
    }
}

void LoadAllData(StrArray &PatientName, StrArray &PatientPassword, StrArray &PatientAge, int &Patientcount,
                 StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                 StrArray &appointmentTime,
                 StrArray &appointmentStatus, StrArray &appointmentEmergency, StrArray &PrescriptionMed, StrArray &MedicineTimes,
                 StrArray &PrescriptionDays, int &appointmentCount,
                 StrArray &Doctors, StrArray &DoctorPassword, StrArray &DoctorEmail, StrArray &DoctorPhone,
                 StrArray &DoctorAddress, StrArray &DoctorQualification, StrArray &DoctorStartTime,
                 StrArray &DoctorEndTime, StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int &DoctorCount,
                 StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount,
                 StrArray &Receptionist, StrArray &ReceptionistPassword, StrArray &ReceptionistEmail,
                 StrArray &ReceptionistPhone, StrArray &ReceptionistAddress, int &ReceptionistCount,
                 StrArray &Pharmacist, StrArray &PharmacistPassword, StrArray &PharmacistEmail,
                 StrArray &PharmacistPhone, StrArray &PharmacistAddress, int &PharmacistCount,
                 StrArray &Medicines, StrArray &MedicinePrices, IntArray &MedicineStock, IntArray &MedicineReorderLevel, int &MedicineCount,
                 StrArray &HistoryPatients, StrArray &HistoryDetails, int &HistoryCount)
{
    LoadPatientsFromFile(PatientName, PatientPassword, PatientAge, Patientcount);
    LoadAppointmentsFromFile(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                             appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount);
    LoadDoctorsFromFile(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                        DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
    LoadDoctorRequests(PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount);
    LoadReceptionistsFromFile(Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount);
    LoadPharmacistsFromFile(Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount);
    LoadMedicinesFromFile(Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount);
    LoadPatientHistory(HistoryPatients, HistoryDetails, HistoryCount);
}

void SaveAllData(const StrArray &PatientName, const StrArray &PatientPassword, const StrArray &PatientAge, int Patientcount,
                 const StrArray &appointmentPatient, const StrArray &appointmentDoctor, const StrArray &appointmentDate,
                 const StrArray &appointmentTime,
                 const StrArray &appointmentStatus, const StrArray &appointmentEmergency, const StrArray &PrescriptionMed, const StrArray &MedicineTimes,
                 const StrArray &PrescriptionDays, int appointmentCount,
                 const StrArray &Doctors, const StrArray &DoctorPassword, const StrArray &DoctorEmail, const StrArray &DoctorPhone,
                 const StrArray &DoctorAddress, const StrArray &DoctorQualification, const StrArray &DoctorStartTime,
                 const StrArray &DoctorEndTime, const StrArray &DocAvailableDays, const StrArray &DoctorSpecialization, int DoctorCount,
                 const StrArray &PendingDoctorNames, const StrArray &PendingDoctorPasswords, int PendingDoctorRequestCount,
                 const StrArray &Receptionist, const StrArray &ReceptionistPassword, const StrArray &ReceptionistEmail,
                 const StrArray &ReceptionistPhone, const StrArray &ReceptionistAddress, int ReceptionistCount,
                 const StrArray &Pharmacist, const StrArray &PharmacistPassword, const StrArray &PharmacistEmail,
                 const StrArray &PharmacistPhone, const StrArray &PharmacistAddress, int PharmacistCount,
                 const StrArray &Medicines, const StrArray &MedicinePrices, const IntArray &MedicineStock, const IntArray &MedicineReorderLevel, int MedicineCount,
                 const StrArray &HistoryPatients, const StrArray &HistoryDetails, int HistoryCount)
{
    SavePatientsToFile(PatientName, PatientPassword, PatientAge, Patientcount);
    SaveAppointmentsToFile(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                           appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount);
    SaveDoctorsToFile(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                      DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
    SaveDoctorRequests(PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount);
    SaveReceptionistsToFile(Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount);
    SavePharmacistsToFile(Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount);
    SaveMedicinesToFile(Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount);
    SavePatientHistory(HistoryPatients, HistoryDetails, HistoryCount);
}

void InitializeDefaults(StrArray &Doctors, StrArray &DoctorPassword, StrArray &DoctorEmail, StrArray &DoctorPhone,
                        StrArray &DoctorAddress, StrArray &DoctorQualification, StrArray &DoctorStartTime,
                        StrArray &DoctorEndTime, StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int &DoctorCount,
                        StrArray &Receptionist, StrArray &ReceptionistPassword, StrArray &ReceptionistEmail,
                        StrArray &ReceptionistPhone, StrArray &ReceptionistAddress, int &ReceptionistCount,
                        StrArray &Pharmacist, StrArray &PharmacistPassword, StrArray &PharmacistEmail,
                        StrArray &PharmacistPhone, StrArray &PharmacistAddress, int &PharmacistCount,
                        StrArray &Medicines, StrArray &MedicinePrices, IntArray &MedicineStock, IntArray &MedicineReorderLevel, int &MedicineCount)
{
    const string defaultDoctors[] = {"Ali", "Hajra"};
    const string defaultDoctorPasswords[] = {"1234", "1234"};
    const string defaultDoctorEmails[] = {"dr.ali@hospital.com", "dr.hajra@hospital.com"};
    const string defaultDoctorPhones[] = {"+92-300-1234567", "+92-321-9876543"};
    const string defaultDoctorAddresses[] = {"123 Medical Plaza, Karachi", "456 Health Center, Lahore"};
    const string defaultDoctorQualifications[] = {"MBBS, FCPS (Cardiology)", "MBBS, MRCP (Endocrinology)"};
    const string defaultStartTimes[] = {"3PM", "7PM"};
    const string defaultEndTimes[] = {"5PM", "9PM"};
    const string defaultAvailableDays[] = {"Monday to Sunday", "Monday to Sunday"};
    const string defaultSpecializations[] = {"Cardiology", "Endocrinology"};
    DoctorCount = 2;
    for (int i = 0; i < DoctorCount; i++)
    {
        Doctors[i] = defaultDoctors[i];
        DoctorPassword[i] = defaultDoctorPasswords[i];
        DoctorEmail[i] = defaultDoctorEmails[i];
        DoctorPhone[i] = defaultDoctorPhones[i];
        DoctorAddress[i] = defaultDoctorAddresses[i];
        DoctorQualification[i] = defaultDoctorQualifications[i];
        DoctorStartTime[i] = defaultStartTimes[i];
        DoctorEndTime[i] = defaultEndTimes[i];
        DocAvailableDays[i] = defaultAvailableDays[i];
        DoctorSpecialization[i] = defaultSpecializations[i];
    }

    const string defaultReceptionists[] = {"Hamza"};
    const string defaultReceptionistPasswords[] = {"recep123"};
    const string defaultReceptionistEmails[] = {"hamza@hospital.com"};
    const string defaultReceptionistPhones[] = {"+92-311-5555555"};
    const string defaultReceptionistAddresses[] = {"Hospital Admin Block"};
    ReceptionistCount = 1;
    for (int i = 0; i < ReceptionistCount; i++)
    {
        Receptionist[i] = defaultReceptionists[i];
        ReceptionistPassword[i] = defaultReceptionistPasswords[i];
        ReceptionistEmail[i] = defaultReceptionistEmails[i];
        ReceptionistPhone[i] = defaultReceptionistPhones[i];
        ReceptionistAddress[i] = defaultReceptionistAddresses[i];
    }

    const string defaultPharmacists[] = {"Umair"};
    const string defaultPharmacistPasswords[] = {"pharm123"};
    const string defaultPharmacistEmails[] = {"umair.pharmacy@hospital.com"};
    const string defaultPharmacistPhones[] = {"+92-333-7777777"};
    const string defaultPharmacistAddresses[] = {"Hospital Pharmacy Wing"};
    PharmacistCount = 1;
    for (int i = 0; i < PharmacistCount; i++)
    {
        Pharmacist[i] = defaultPharmacists[i];
        PharmacistPassword[i] = defaultPharmacistPasswords[i];
        PharmacistEmail[i] = defaultPharmacistEmails[i];
        PharmacistPhone[i] = defaultPharmacistPhones[i];
        PharmacistAddress[i] = defaultPharmacistAddresses[i];
    }

    const string defaultMedicines[] = {"Captopril", "Esomeprazole", "Furosemide", "Metoclopramide", "Cimetidine",
                                       "Predenisolone", "Insulin", "Metformin", "Ibuprofen", "Methotrexate"};
    const string defaultMedicinePrices[] = {"Rs.200", "Rs.250", "Rs.275", "Rs.300", "Rs.340",
                                            "Rs.370", "Rs.390", "Rs.420", "Rs.450", "Rs.500"};
    MedicineCount = 10;
    for (int i = 0; i < MedicineCount; i++)
    {
        Medicines[i] = defaultMedicines[i];
        MedicinePrices[i] = defaultMedicinePrices[i];
        MedicineStock[i] = 75;
        MedicineReorderLevel[i] = 25;
    }
}

/*____________________Functions______________________*/

// Registers the Patients
bool SignUp(StrArray &PatientName, StrArray &PatientPassword, StrArray &PatientAge, int &Patientcount,
            const string &PN, const string &PP, const string &PA)
{
    if (Patientcount < UserSize)
    {
        for (int i = 0; i < Patientcount; i++)
        {
            if (PatientName[i] == PN && PatientPassword[i] == PP)
            {
                return false;
            }
        }
        PatientName[Patientcount] = PN;
        PatientPassword[Patientcount] = PP;
        PatientAge[Patientcount] = PA;
        Patientcount++;
        return true;
    }
    return false;
}
bool DoctorSignup(StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount,
                  const StrArray &Doctors, int DoctorCount,
                  const string &DN, const string &DP)
{
    if (PendingDoctorRequestCount >= UserSize)
    {
        return false;
    }

    string normalized = ToUpper(DN);

    for (int i = 0; i < DoctorCount; i++)
    {
        if (ToUpper(Doctors[i]) == normalized)
        {
            return false;
        }
    }

    for (int i = 0; i < PendingDoctorRequestCount; i++)
    {
        if (ToUpper(PendingDoctorNames[i]) == normalized)
        {
            return false;
        }
    }

    PendingDoctorNames[PendingDoctorRequestCount] = DN;
    PendingDoctorPasswords[PendingDoctorRequestCount] = DP;
    PendingDoctorRequestCount++;
    return true;
}
bool DocLogin(const StrArray &Doctors, const StrArray &DoctorPassword, int DoctorCount, const string &DN, const string &DP)
{
    for (int i = 0; i < DoctorCount; i++)
    {
        if (Doctors[i] == DN && DoctorPassword[i] == DP)
        {
            return true;
        }
    }
    return false;
}

// Matches the Patients credentials
string Login(const StrArray &PatientName, const StrArray &PatientPassword, int Patientcount, const string &PN, const string &PP)
{
    for (int i = 0; i < Patientcount; i++)
    {
        if (PatientName[i] == PN && PatientPassword[i] == PP)
        {
            return PatientName[i];
        }
    }
    return "Undefined";
}

// Show All the Appointments
void ViewAppointments(const StrArray &appointmentPatient, const StrArray &appointmentDate,
                      const StrArray &appointmentTime, const StrArray &appointmentDoctor, int appointmentCount)
{
    if (appointmentCount == 0)
    {
        showInfo("No appointments found.");
        return;
    }
    
    drawTableHeader("Patient Name", "Doctor", "Date", "Time");
    
    for (int i = 0; i < appointmentCount; i++)
    {
        string time = appointmentTime[i].empty() ? "Not Set" : appointmentTime[i];
        drawTableRow(appointmentPatient[i], "Dr. " + appointmentDoctor[i], appointmentDate[i], time);
    }
    
    drawTableFooter();
    useYellowColour();
    cout << "\n📊 Total Appointments: " << appointmentCount << endl;
    useWhiteColour();
}

// Show Pharmacists
void ViewPharmacist(const StrArray &Pharmacist, const StrArray &PharmacistEmail,
                    const StrArray &PharmacistPhone, const StrArray &PharmacistAddress, int PharmacistCount)
{
    if (PharmacistCount == 0)
    {
        useRedColour();
        cout << "No pharmacists in the system." << endl;
        useWhiteColour();
        return;
    }
    
    useCyanColour();
    cout << "\n╔════════════════════════════════════════════════════════════════════════╗" << endl;
    useMagentaColour();
    cout << "║                    💊  PHARMACIST DIRECTORY  💊                       ║" << endl;
    useCyanColour();
    cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
    useWhiteColour();
    
    for (int i = 0; i < PharmacistCount; i++)
    {
        useGreenColour();
        cout << "║  📋 Pharmacist #" << (i + 1) << " - " << Pharmacist[i];
        cout << string(52 - Pharmacist[i].length(), ' ') << "║" << endl;
        useCyanColour();
        cout << "║  " << string(70, '-') << "║" << endl;
        useYellowColour();
        cout << "║    📧 Email:   " << left << setw(53) << (PharmacistEmail[i].empty() ? "Not provided" : PharmacistEmail[i]) << "║" << endl;
        cout << "║    📞 Phone:   " << left << setw(53) << (PharmacistPhone[i].empty() ? "Not provided" : PharmacistPhone[i]) << "║" << endl;
        cout << "║    🏠 Address: " << left << setw(53) << (PharmacistAddress[i].empty() ? "Not provided" : PharmacistAddress[i]) << "║" << endl;
        
        if (i < PharmacistCount - 1)
        {
            useCyanColour();
            cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
        }
    }
    
    useCyanColour();
    cout << "╚════════════════════════════════════════════════════════════════════════╝" << endl;
    useWhiteColour();
    cout << "\nPress any key to continue...";
    _getch();
}

// Show Receptionists
void ViewReceptionist(const StrArray &Receptionist, const StrArray &ReceptionistEmail,
                      const StrArray &ReceptionistPhone, const StrArray &ReceptionistAddress, int ReceptionistCount)
{
    if (ReceptionistCount == 0)
    {
        useRedColour();
        cout << "No receptionists in the system." << endl;
        useWhiteColour();
        return;
    }
    
    useCyanColour();
    cout << "\n╔════════════════════════════════════════════════════════════════════════╗" << endl;
    useMagentaColour();
    cout << "║                   👥  RECEPTIONIST DIRECTORY  👥                      ║" << endl;
    useCyanColour();
    cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
    useWhiteColour();
    
    for (int i = 0; i < ReceptionistCount; i++)
    {
        useGreenColour();
        cout << "║  📋 Receptionist #" << (i + 1) << " - " << Receptionist[i];
        cout << string(49 - Receptionist[i].length(), ' ') << "║" << endl;
        useCyanColour();
        cout << "║  " << string(70, '-') << "║" << endl;
        useYellowColour();
        cout << "║    📧 Email:   " << left << setw(53) << (ReceptionistEmail[i].empty() ? "Not provided" : ReceptionistEmail[i]) << "║" << endl;
        cout << "║    📞 Phone:   " << left << setw(53) << (ReceptionistPhone[i].empty() ? "Not provided" : ReceptionistPhone[i]) << "║" << endl;
        cout << "║    🏠 Address: " << left << setw(53) << (ReceptionistAddress[i].empty() ? "Not provided" : ReceptionistAddress[i]) << "║" << endl;
        
        if (i < ReceptionistCount - 1)
        {
            useCyanColour();
            cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
        }
    }
    
    useCyanColour();
    cout << "╚════════════════════════════════════════════════════════════════════════╝" << endl;
    useWhiteColour();
    cout << "\nPress any key to continue...";
    _getch();
}

// Show Doctors
void ViewDoctors(const StrArray &Doctors, const StrArray &DoctorEmail, const StrArray &DoctorPhone,
                 const StrArray &DoctorAddress, const StrArray &DoctorQualification,
                 const StrArray &DoctorStartTime, const StrArray &DoctorEndTime,
                 const StrArray &DocAvailableDays, const StrArray &DoctorSpecialization, int DoctorCount)
{
    if (DoctorCount == 0)
    {
        useRedColour();
        cout << "No doctors in the system." << endl;
        useWhiteColour();
        return;
    }
    
    useCyanColour();
    cout << "\n╔════════════════════════════════════════════════════════════════════════╗" << endl;
    useMagentaColour();
    cout << "║                    👨‍⚕️  DOCTOR DIRECTORY  👨‍⚕️                         ║" << endl;
    useCyanColour();
    cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
    useWhiteColour();
    
    for (int i = 0; i < DoctorCount; i++)
    {
        useGreenColour();
        cout << "║  📋 Dr. " << Doctors[i];
        cout << string(61 - Doctors[i].length(), ' ') << "║" << endl;
        useCyanColour();
        cout << "║  " << string(70, '-') << "║" << endl;
        useYellowColour();
        string spec = DoctorSpecialization[i].empty() ? "General Medicine" : DoctorSpecialization[i];
        cout << "║    🔬 Specialization: " << left << setw(46) << spec << "║" << endl;
        cout << "║    🎓 Qualification:  " << left << setw(46) << (DoctorQualification[i].empty() ? "Not provided" : DoctorQualification[i]) << "║" << endl;
        cout << "║    📧 Email:          " << left << setw(46) << (DoctorEmail[i].empty() ? "Not provided" : DoctorEmail[i]) << "║" << endl;
        cout << "║    📞 Phone:          " << left << setw(46) << (DoctorPhone[i].empty() ? "Not provided" : DoctorPhone[i]) << "║" << endl;
        cout << "║    🏠 Address:        " << left << setw(46) << (DoctorAddress[i].empty() ? "Not provided" : DoctorAddress[i]) << "║" << endl;
        useWhiteColour();
        string schedule = DoctorStartTime[i] + " - " + DoctorEndTime[i];
        cout << "║    🕐 Working Hours:  " << left << setw(46) << schedule << "║" << endl;
        cout << "║    📅 Available Days: " << left << setw(46) << DocAvailableDays[i] << "║" << endl;
        
        if (i < DoctorCount - 1)
        {
            useCyanColour();
            cout << "╠════════════════════════════════════════════════════════════════════════╣" << endl;
        }
    }
    
    useCyanColour();
    cout << "╚════════════════════════════════════════════════════════════════════════╝" << endl;
    useWhiteColour();
    cout << "\nPress any key to continue...";
    _getch();
}

// Changes the text colors
// Clears screen to increase Readability
void clearScreen()
{
    cout << "Press Any Key to Continue..";
    getch();
    system("cls");
}

// Prints the Main menu
int OpeningMenu()
{
    
    cout << " Welcome to the Hospital Management System \n";

    cout << "1. Log In \n";
    cout << "2. Sign Up \n";
    cout << "3. Exit \n";
    int Option = SafeIntInput("Enter your choice: ", 1, 3);
    return Option;
}
int Menu()
{
    int width = 50;
    drawBox(width);
    useYellowColour();
    drawBoxSides("SELECT USER TYPE", width);
    useCyanColour();
    cout << "╠"; // Left T
    for (int i = 0; i < width - 2; i++)
        cout << "═";
    cout << "╣" << endl; // Right T
    
    // Option 1
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useGreenColour();
    cout << "[1]";
    useWhiteColour();
    cout << " Admin";
    cout << string(width - 14, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Option 2
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useGreenColour();
    cout << "[2]";
    useWhiteColour();
    cout << " Doctor";
    cout << string(width - 15, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Option 3
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useGreenColour();
    cout << "[3]";
    useWhiteColour();
    cout << " Receptionist";
    cout << string(width - 21, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Option 4
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useGreenColour();
    cout << "[4]";
    useWhiteColour();
    cout << " Patient";
    cout << string(width - 16, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Option 5
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useGreenColour();
    cout << "[5]";
    useWhiteColour();
    cout << " Pharmacist";
    cout << string(width - 19, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    // Option 6
    useCyanColour();
    cout << "║";
    useWhiteColour();
    cout << "   ";
    useRedColour();
    cout << "[6]";
    useWhiteColour();
    cout << " Exit";
    cout << string(width - 13, ' ');
    useCyanColour();
    cout << "║" << endl;
    
    drawBoxBottom(width);
    
    useCyanColour();
    useWhiteColour();
    int option = SafeIntInput("\nEnter your choice: ", 1, 6);
    return option;
}

// Book appointment with doctor and assign the next available slot
bool BookAppointment(StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                     StrArray &appointmentTime, StrArray &appointmentStatus, StrArray &appointmentEmergency,
                     StrArray &PrescriptionMed, StrArray &MedicineTimes, StrArray &PrescriptionDays, int &appointmentCount,
                     const StrArray &Doctors, const StrArray &DoctorStartTime, const StrArray &DoctorEndTime,
                     int DoctorCount, const string &name, const string &doctor, const string &date, bool isEmergency = false)
{
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentPatient[i] == name && appointmentDate[i] == date)
        {
            return false; // already scheduled
        }
    }

    if (appointmentCount >= UserSize)
    {
        return false;
    }

    int doctorIndex = FindDoctorIndex(Doctors, DoctorCount, doctor);
    if (doctorIndex == -1)
    {
        return false;
    }

    int startMinutes = ParseTimeToMinutes(DoctorStartTime[doctorIndex]);
    int endMinutes = ParseTimeToMinutes(DoctorEndTime[doctorIndex]);
    if (startMinutes == -1 || endMinutes == -1)
    {
        return false;
    }

    int bookedSlots = CountAppointmentsForDoctorOnDate(appointmentDoctor, appointmentDate, appointmentCount, doctor, date);
    int slotStart = startMinutes + bookedSlots * APPOINTMENT_SLOT_MINUTES;
    if (slotStart + APPOINTMENT_SLOT_MINUTES > endMinutes)
    {
        return false;
    }

    appointmentPatient[appointmentCount] = name;
    appointmentDoctor[appointmentCount] = doctor;
    appointmentDate[appointmentCount] = date;
    appointmentTime[appointmentCount] = FormatMinutesToTime(slotStart);
    appointmentStatus[appointmentCount] = isEmergency ? "Emergency-Pending" : "Pending";
    appointmentEmergency[appointmentCount] = isEmergency ? "Yes" : "No";
    PrescriptionMed[appointmentCount].clear();
    MedicineTimes[appointmentCount].clear();
    PrescriptionDays[appointmentCount].clear();
    appointmentCount++;
    return true;
}

// Cancel an appointment
bool CancelAppointment(StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                       StrArray &appointmentTime, StrArray &appointmentStatus, StrArray &appointmentEmergency,
                       StrArray &PrescriptionMed, StrArray &MedicineTimes, StrArray &PrescriptionDays, int &appointmentCount,
                       const string &name, const string &date)
{
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentPatient[i] == name && appointmentDate[i] == date)
        {

            // Deleting the record of appointment
            for (int j = i; j < appointmentCount - 1; j++)
            {
                appointmentPatient[j] = appointmentPatient[j + 1];
                appointmentDoctor[j] = appointmentDoctor[j + 1];
                appointmentEmergency[j] = appointmentEmergency[j + 1];
                appointmentDate[j] = appointmentDate[j + 1];
                appointmentTime[j] = appointmentTime[j + 1];
                appointmentStatus[j] = appointmentStatus[j + 1];
                PrescriptionMed[j] = PrescriptionMed[j + 1];
                MedicineTimes[j] = MedicineTimes[j + 1];
                PrescriptionDays[j] = PrescriptionDays[j + 1];
            }

            appointmentCount--;
            return true;
        }
    }
    return false; // not found
}

// Search the patient in appointments
bool SearchPatient(const StrArray &appointmentPatient, int appointmentCount, const string &name)
{
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentPatient[i] == name)
        {
            return true;
        }
    }
    return false;
}

// For adding new Medicines
void addMedicine(StrArray &Medicines, StrArray &MedicinePrices, int &MedicineCount)
{
    for (int i = MedicineCount; i < UserSize; i++)
    {

        Medicines[i] = PromptLine("Enter the Medicine : ");
        MedicineCount++;

        MedicinePrices[i] = PromptLine("Enter the price of Medicine : ");
        int choice;
        cout << "Press 0 to Exit. " << endl;
        cout << "Press 1 to Enter more medicines : " << endl;
        cin >> choice;
        if (choice == 0)
        {
            break;
        }
    }
}

// for updating the medicine prices
void updateMedicine(StrArray &Medicines, StrArray &MedicinePrices, int MedicineCount, const string &name)
{

    for (int i = 0; i < MedicineCount; i++)
    {
        if (Medicines[i] == name)
        {
            cout << Medicines[i] << " , " << MedicinePrices[i];
            cout << endl;
            MedicinePrices[i] = PromptLine("Enter the new Price : ");
        }
    }
}

// To delete a Medicine
bool deleteMedicine(StrArray &Medicines, StrArray &MedicinePrices, int &MedicineCount, const string &name)
{
    for (int i = 0; i < MedicineCount; i++)
    {
        if (Medicines[i] == name)
        {
            for (int j = i; j < MedicineCount; j++)
            {
                Medicines[j] = Medicines[j + 1];
                MedicinePrices[j] = MedicinePrices[j + 1];
            }
            MedicineCount--;
            return true;
        }
    }
    return false;
}

// Search Medicine in the existing medicines
bool searchMedicine(const StrArray &Medicines, int MedicineCount, const string &name)
{
    for (int i = 0; i < MedicineCount; i++)
    {
        if (Medicines[i] == name)
        {
            return true;
        }
    }

    return false;
}

// Shows The Medicines
void viewAllMedicine(const StrArray &Medicines, const StrArray &MedicinePrices, int MedicineCount)
{
    useCyanColour();
    cout << "\n╔════════════════════════════════════════════════╗" << endl;
    useMagentaColour();
    cout << "║         💊  MEDICINE INVENTORY  💊          ║" << endl;
    useCyanColour();
    cout << "╠════════════════════════════════════════════════╣" << endl;
    
    for (int i = 0; i < MedicineCount; i++)
    {
        useGreenColour();
        cout << "║  " << (i + 1) << ". " << left << setw(25) << Medicines[i];
        useYellowColour();
        cout << setw(12) << MedicinePrices[i];
        useCyanColour();
        cout << "  ║" << endl;
    }
    
    useCyanColour();
    cout << "╚════════════════════════════════════════════════╝" << endl;
    useYellowColour();
    cout << "📊 Total Medicines: " << MedicineCount << endl;
    useWhiteColour();
}

// Manages the medicines
void ManageInventory(StrArray &Medicines, StrArray &MedicinePrices, int &MedicineCount)
{
    int choice = 0;
    while (choice != 6)
    {
        cout << "1. Add Medicine\n";
        cout << "2. Update Medicine\n";
        cout << "3. Delete Medicine\n";
        cout << "4. Search Medicine\n";
        cout << "5. View all Medicines\n";
        cout << "6. Back to menu\n ";
        choice = SafeIntInput("Enter your choice: ", 1, 6);
        if (choice == 1)
        {
            addMedicine(Medicines, MedicinePrices, MedicineCount);
        }
        else if (choice == 2)
        {
            string name = PromptLine("Enter the name of the Medicine to be updated : ");
            updateMedicine(Medicines, MedicinePrices, MedicineCount, name);
        }
        else if (choice == 3)
        {
            string name = PromptLine("Enter the name of the Medicine to be deleted : ");

            deleteMedicine(Medicines, MedicinePrices, MedicineCount, name);
        }
        else if (choice == 4)
        {
            string name = PromptLine("Enter the name of the Medicine to be Searched : ");
            if (searchMedicine(Medicines, MedicineCount, name))
            {
                showSuccess("Medicine Found!");
                useGreenColour();
                cout << "💊 " << name << " is available in inventory" << endl;
                useWhiteColour();
            }
            else
            {
                showError("Medicine not Found.");
            }
        }

        else if (choice == 5)
        {
            viewAllMedicine(Medicines, MedicinePrices, MedicineCount);
        }
        else if (choice == 6)
        {
            break;
        }
        clearScreen();
    }
}

// View Doctors Prescriptions
void ViewPrescriptions(const StrArray &PrescriptionMed, const StrArray &MedicineTimes, const StrArray &PrescriptionDays, int appointmentCount)
{
    for (int i = 0; i < appointmentCount; i++)
    {
        cout << PrescriptionMed[i] << " " << MedicineTimes[i] << " times a day " << " for " << PrescriptionDays[i] << endl;
    }
}

// To write a prescription
void Prescriptions(const StrArray &appointmentPatient, StrArray &PrescriptionMed, StrArray &MedicineTimes,
                   StrArray &PrescriptionDays, int appointmentCount)
{
    int choice = 1;
    while (choice != 0)
    {

        cout << "Press 1 to enter Prescription ";
        cout << "Press 0 to Exit : ";
        choice = SafeIntInput("", 0, 1);

        if (choice == 1)
        {
            string name = PromptLine("Enter the Patient Name : ");
            for (int i = 0; i < appointmentCount; i++)
            {
                if (appointmentPatient[i] == name)
                {

                    PrescriptionMed[i] = PromptLine("Enter the name of Medicine : ");
                    MedicineTimes[i] = PromptLine("Enter the no. of times the medicine should be taken in a day : ");
                    PrescriptionDays[i] = PromptLine("Enter the no. of days the medicine should be taken : ");
                }
            }
        }
        if (choice == 0)
        {
            break;
        }
        clearScreen();
    }
}

// Update Doctors timing
void UpdateTiming(StrArray &Doctors, StrArray &DoctorStartTime, StrArray &DoctorEndTime, StrArray &DocAvailableDays, int DoctorCount, const string &name)
{
    for (int i = 0; i < DoctorCount; i++)
    {
        if (Doctors[i] == name)
        {
            DoctorStartTime[i] = PromptLine("Enter the New Starting Time : ");
            DoctorEndTime[i] = PromptLine("Enter the New Ending Time : ");
            DocAvailableDays[i] = PromptLine("Enter the Days Available  : ");
        }
    }
}

// Marks the status of the Appointments
void MarkStatus(const StrArray &appointmentDoctor, StrArray &appointmentStatus, const StrArray &appointmentPatient, int appointmentCount)
{
    string name = PromptLine("Enter the Doctor's name : ");
    for (int i = 0; i < appointmentCount; i++)
    {
        if (appointmentDoctor[i] == name)
        {

            appointmentStatus[i] = PromptLine("Mark the Status For Patient " + appointmentPatient[i] + ": ");
        }
    }
}

void ReviewDoctorSignupRequests(StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount,
                                StrArray &Doctors, StrArray &DoctorPassword, StrArray &DoctorEmail, StrArray &DoctorPhone,
                                StrArray &DoctorAddress, StrArray &DoctorQualification, StrArray &DoctorStartTime,
                                StrArray &DoctorEndTime, StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int &DoctorCount)
{
    if (PendingDoctorRequestCount == 0)
    {
        cout << "No pending doctor signup requests." << endl;
        return;
    }

    while (PendingDoctorRequestCount > 0)
    {
        cout << "Pending Doctor Signup Requests:" << endl;
        for (int i = 0; i < PendingDoctorRequestCount; i++)
        {
            cout << i + 1 << ". " << PendingDoctorNames[i] << endl;
        }

        int selection = SafeIntInput("Enter request number to review (0 to exit): ", 0, PendingDoctorRequestCount);
        if (selection == 0)
        {
            break;
        }
        if (selection < 1 || selection > PendingDoctorRequestCount)
        {
            cout << "Invalid selection." << endl;
            continue;
        }

        int index = selection - 1;
        int action = SafeIntInput("1. Approve\n2. Reject\nChoose option: ", 1, 2);
        if (action == 1)
        {
            if (DoctorCount >= UserSize)
            {
                cout << "Cannot approve request. Doctor limit reached." << endl;
                continue;
            }

            useGreenColour();
            cout << "\n=== Doctor Profile Setup ===" << endl;
            useWhiteColour();
            
            string email = PromptLine("Enter email address: ");
            string phone = PromptLine("Enter phone number (e.g., +92-300-1234567): ");
            string address = PromptLine("Enter address: ");
            string qualification = PromptLine("Enter qualification (e.g., MBBS, FCPS): ");
            string startTime = PromptLine("Enter starting time (e.g., 3PM): ");
            string endTime = PromptLine("Enter ending time (e.g., 5PM): ");
            string availableDays = PromptLine("Enter available days : ");
            string specialization = PromptLine("Enter specialization : ");
            if (specialization.empty())
            {
                specialization = "General Medicine";
            }

            Doctors[DoctorCount] = PendingDoctorNames[index];
            DoctorPassword[DoctorCount] = PendingDoctorPasswords[index];
            DoctorEmail[DoctorCount] = email;
            DoctorPhone[DoctorCount] = phone;
            DoctorAddress[DoctorCount] = address;
            DoctorQualification[DoctorCount] = qualification;
            DoctorStartTime[DoctorCount] = startTime;
            DoctorEndTime[DoctorCount] = endTime;
            DocAvailableDays[DoctorCount] = availableDays;
            DoctorSpecialization[DoctorCount] = specialization;
            DoctorCount++;
            
            // Immediately save to file
            SaveDoctorsToFile(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                              DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
            
            useGreenColour();
            cout << "✓ Doctor signup approved successfully!" << endl;
            useWhiteColour();

            for (int i = index; i < PendingDoctorRequestCount - 1; i++)
            {
                PendingDoctorNames[i] = PendingDoctorNames[i + 1];
                PendingDoctorPasswords[i] = PendingDoctorPasswords[i + 1];
            }
            PendingDoctorRequestCount--;
        }
        else if (action == 2)
        {
            for (int i = index; i < PendingDoctorRequestCount - 1; i++)
            {
                PendingDoctorNames[i] = PendingDoctorNames[i + 1];
                PendingDoctorPasswords[i] = PendingDoctorPasswords[i + 1];
            }
            PendingDoctorRequestCount--;
            cout << "Doctor signup request rejected." << endl;
        }
        else
        {
            cout << "Invalid option." << endl;
        }
    }
}

// Shows the Doctor's menu and all the features
void DoctorInterface(StrArray &Doctors, StrArray &DoctorPassword, StrArray &DoctorStartTime,
                     StrArray &DoctorEndTime, StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int &DoctorCount,
                     StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                     StrArray &appointmentTime, StrArray &appointmentStatus, StrArray &PrescriptionMed, StrArray &MedicineTimes,
                     StrArray &PrescriptionDays, int appointmentCount,
                     StrArray &HistoryPatients, StrArray &HistoryDetails, int &HistoryCount,
                     StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount)
{
    int option1 = 0;
    int option = 0;
    while (option1 != 3)
    {
        cout << "1. Sign Up \n";
        cout << "2. Log In \n";
        cout << "3. Exit \n";
        option1 = SafeIntInput("Enter your choice: ", 1, 3);
        if (option1 == 1)
        {
            string Doctor = PromptLine("Enter Your Name : ");
            string DP = PromptLine("Create Your Password : ");
            if (DoctorSignup(PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                             Doctors, DoctorCount, Doctor, DP))
            {
                cout << "Signup request submitted. Await admin approval.\n ";
            }
            else
            {
                cout << "Sign up request failed. Name might already exist or request queue is full." << endl;
            }
        }
        else if (option1 == 2)
        {
            string Doctor = PromptLine("Enter Your Name : ");
            string DP = PromptLine("Enter Your Password : ");
            if (DocLogin(Doctors, DoctorPassword, DoctorCount, Doctor, DP))
            {

                clearScreen();
                int loggedInIndex = FindDoctorIndex(Doctors, DoctorCount, Doctor);
                cout << "Welcome Dr. " << Doctor;
                if (loggedInIndex != -1 && !DoctorSpecialization[loggedInIndex].empty())
                {
                    cout << " (" << DoctorSpecialization[loggedInIndex] << ")";
                }
                cout << endl;
                while (option != 6)
                {
                    printHeader();
                    string doctorOptions[] = {
                        "📋 View Appointments",
                        "✅ Mark Appointment Status",
                        "⏰ Update Available Timing",
                        "💊 Write Prescription",
                        "📊 View Performance Dashboard",
                        "🚪 Log Out"
                    };
                    displayMenuBox("👨‍⚕️  DOCTOR DASHBOARD  👨‍⚕️", doctorOptions, 6);
                    option = SafeIntInput("Enter your choice: ", 1, 6);
                    if (option == 1)
                    {
                        string name = PromptLine("Enter the Doctor's name : ");
                        int count = 0;
                        bool headerPrinted = false;
                        
                        for (int i = 0; i < appointmentCount; i++)
                        {
                            if (appointmentDoctor[i] == name)
                            {
                                if (!headerPrinted)
                                {
                                    drawTableHeader("Patient Name", "Date", "Time", "Status");
                                    headerPrinted = true;
                                }
                                string time = appointmentTime[i].empty() ? "Not Set" : appointmentTime[i];
                                string status = appointmentStatus[i].empty() ? "Pending" : appointmentStatus[i];
                                drawTableRow(appointmentPatient[i], appointmentDate[i], time, status);
                                count++;
                            }
                        }
                        
                        if (headerPrinted)
                        {
                            drawTableFooter();
                            useGreenColour();
                            cout << "\n📊 Total Appointments: " << count << endl;
                            useWhiteColour();
                        }
                        else
                        {
                            showInfo("No appointments found for Dr. " + name);
                        }
                    }

                    else if (option == 2)
                    {
                        MarkStatus(appointmentDoctor, appointmentStatus, appointmentPatient, appointmentCount);
                    }

                    else if (option == 3)
                    {
                        string name = PromptLine("Enter Doctor's name : ");
                        UpdateTiming(Doctors, DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorCount, name);
                    }
                    else if (option == 4)
                    {
                        Prescriptions(appointmentPatient, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount);
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        string patient;
                        cout << "Enter patient name to log history: ";
                        getline(cin, patient);
                        patient = ToUpper(patient);
                        for (int i = 0; i < appointmentCount; i++)
                        {
                            if (ToUpper(appointmentPatient[i]) == patient)
                            {
                                if (HistoryCount < UserSize)
                                {
                                    HistoryPatients[HistoryCount] = appointmentPatient[i];
                                    HistoryDetails[HistoryCount] = "Doctor: " + appointmentDoctor[i] +
                                                                     " Date: " + appointmentDate[i] +
                                                                     (appointmentTime[i].empty() ? "" : (" Time: " + appointmentTime[i])) +
                                                                     " Prescription: " + PrescriptionMed[i] +
                                                                     " Times: " + MedicineTimes[i] +
                                                                     " Days: " + PrescriptionDays[i];
                                    HistoryCount++;
                                }
                                break;
                            }
                        }
                    }

                    else if (option == 5)
                    {
                        // View Performance Dashboard
                        showDoctorPerformance(Doctor, appointmentDoctor, appointmentDate, appointmentStatus, appointmentCount);
                    }
                    
                    else if (option == 6)
                    {
                        break;
                    }
                    clearScreen();
                }
                
            }
            else
            {
                cout << "Invalid Username Password " << endl;
            }
        }
        else if (option1 == 3)
        {
            cout << "Good Bye " << endl;
            break;
        }
    }
    
   
}

// Shows the Patient' Menu with its Features
void PatientInterface(StrArray &PatientName, StrArray &PatientPassword, StrArray &PatientAge, int &Patientcount,
                      StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                      StrArray &appointmentTime, StrArray &appointmentStatus, StrArray &appointmentEmergency,
                      StrArray &PrescriptionMed, StrArray &MedicineTimes, StrArray &PrescriptionDays, int &appointmentCount,
                      StrArray &Doctors, StrArray &DoctorStartTime, StrArray &DoctorEndTime,
                      StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int DoctorCount,
                      const StrArray &Medicines, const StrArray &MedicinePrices, int MedicineCount)
{

    int option = 0;
    while (option != 3)
    {
        printHeader();
        cout << "1. Sign Up " << endl;
        cout << "2. Log In " << endl;
        cout << "3. Exit " << endl;
        option = SafeIntInput("Enter your choice: ", 1, 3);
        if (option == 1)
        {
            string Patient = PromptLine("Enter Your Name : ");
            string PTPassword = PromptLine("Create Your Password : ");
            string Age = PromptLine("Enter Your Age : ");
            if (SignUp(PatientName, PatientPassword, PatientAge, Patientcount, Patient, PTPassword, Age))
            {
                showSuccess("Account Created Successfully!");
                useGreenColour();
                cout << "🎉 Welcome " << Patient << "!" << endl;
                useWhiteColour();
            }
            else
            {
                showError("Sign Up Failed - Account may already exist.");
            }
        }
        else if (option == 2)
        {
            string Patient = PromptLine("Enter Your Name : ");
            string PTPassword = PromptLine("Enter Your Password : ");
            string result;
            result = Login(PatientName, PatientPassword, Patientcount, Patient, PTPassword);
            if (result != "Undefined")
            {

                clearScreen();
                
                // Show reminders on login
                showUpcomingAppointments(result, appointmentPatient, appointmentDoctor, appointmentDate, 
                                         appointmentTime, appointmentStatus, appointmentCount);
                
                cout << "\nWelcome " << result << "!" << endl;

                int option2 = 0;
                while (option2 != 7)
                {
                    printHeader();
                    string patientOptions[] = {
                        "📅 Book Appointment",
                        "❌ Cancel Appointment",
                        "👨‍⚕️ View Doctors Details & Timings",
                        "🧾 Generate Bill",
                        "📋 View Appointment History",
                        "🚨 Book Emergency Appointment",
                        "🚪 Log Out"
                    };
                    displayMenuBox("🏥  PATIENT DASHBOARD  🏥", patientOptions, 7);
                    option2 = SafeIntInput("Enter your choice: ", 1, 7);
                    if (option2 == 1)
                    {
                        string PTName = PromptLine("Enter Patient Name : ");
                        string date = PromptLine("Enter the date for appointment (YYYY-MM-DD) : ");
                        string DocName = PromptLine("Enter The Doctor's Name : ");
                        if (BookAppointment(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime,
                                             appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                             appointmentCount,
                                             Doctors, DoctorStartTime, DoctorEndTime, DoctorCount,
                                             PTName, DocName, date, false))
                        {
                            showSuccess("Appointment Booked Successfully!");
                            useGreenColour();
                            cout << "ℹ Dr. " << DocName << " on " << date << endl;
                            useWhiteColour();
                        }
                        else
                        {
                            showError("Appointment already exists or invalid data.");
                        }
                    }
                    else if (option2 == 2)
                    {

                        string PTName = PromptLine("Enter Patient Name : ");
                        string date = PromptLine("Enter the date for appointment (YYYY-MM-DD) : ");
                        if (CancelAppointment(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime,
                                              appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                              appointmentCount, PTName, date))
                        {
                            showSuccess("Appointment Cancelled Successfully!");
                        }
                        else
                        {
                            showError("Appointment Not Found.");
                        }
                    }
                    else if (option2 == 3)
                    {
                        // Simple doctor list for patients
                        useCyanColour();
                        cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
                        useMagentaColour();
                        cout << "║              👨‍⚕️  AVAILABLE DOCTORS  👨‍⚕️               ║" << endl;
                        useCyanColour();
                        cout << "╠════════════════════════════════════════════════════════╣" << endl;
                        
                        for (int i = 0; i < DoctorCount; i++)
                        {
                            useGreenColour();
                            cout << "║  " << (i + 1) << ". Dr. " << left << setw(20) << Doctors[i];
                            useYellowColour();
                            string spec = DoctorSpecialization[i].empty() ? "General" : DoctorSpecialization[i];
                            cout << "[" << setw(18) << spec << "]";
                            useCyanColour();
                            cout << " ║" << endl;
                        }
                        
                        useCyanColour();
                        cout << "╚════════════════════════════════════════════════════════╝" << endl;
                        useWhiteColour();
                        
                        string name = PromptLine("Enter the doctor's name to view details of Dr. ");
                        for (int i = 0; i < DoctorCount; i++)
                        {
                            if (Doctors[i] == name)
                            {
                                cout << "Specialization : " << (DoctorSpecialization[i].empty() ? "General Medicine" : DoctorSpecialization[i]) << endl;
                                cout << "Start : " << DoctorStartTime[i] << " , End :  " << DoctorEndTime[i] << " , Days :  " << DocAvailableDays[i] << endl;
                            }
                        }
                    }
                    else if (option2 == 4)
                    {
                        if (GeneratePatientBillPDF(result, appointmentPatient, PrescriptionMed, appointmentCount,
                                                    Medicines, MedicinePrices, MedicineCount))
                        {
                            showSuccess("Bill generated successfully!");
                            useGreenColour();
                            cout << "💾 Saved as PDF in current directory" << endl;
                            useWhiteColour();
                        }
                        else
                        {
                            showError("Failed to generate bill.");
                        }
                    }
                    else if (option2 == 5)
                    {
                        // View Appointment History
                        showPatientHistory(result, appointmentPatient, appointmentDoctor, appointmentDate,
                                           appointmentTime, appointmentStatus, appointmentEmergency, appointmentCount);
                    }
                    else if (option2 == 6)
                    {
                        // Book Emergency Appointment
                        string PTName = PromptLine("Enter Patient Name : ");
                        string date = PromptLine("Enter the date for EMERGENCY appointment (YYYY-MM-DD) : ");
                        string DocName = PromptLine("Enter The Doctor's Name : ");
                        if (BookAppointment(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime,
                                             appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                             appointmentCount,
                                             Doctors, DoctorStartTime, DoctorEndTime, DoctorCount,
                                             PTName, DocName, date, true))
                        {
                            useRedColour();
                            showSuccess("🚨 EMERGENCY Appointment Booked Successfully!");
                            useYellowColour();
                            cout << "⚠️ Emergency Fee: Rs." << EMERGENCY_CONSULTATION_FEE << endl;
                            useGreenColour();
                            cout << "ℹ Dr. " << DocName << " on " << date << endl;
                            useWhiteColour();
                        }
                        else
                        {
                            showError("Emergency Appointment booking failed.");
                        }
                    }
                    else if (option2 == 7)
                    {
                        break;
                    }
                    clearScreen();
                }
            }
            else
            {
                cout << "Invalid Username Password " << endl;
            }
        }
        else if (option == 3)
        {
            cout << "Good Bye " << endl;
            break;
        }
        clearScreen();
    }
}

// Shows the Admin' menu And all the Features
void AdminInterface(StrArray &Doctors, StrArray &DoctorPassword, StrArray &DoctorEmail, StrArray &DoctorPhone,
                    StrArray &DoctorAddress, StrArray &DoctorQualification, StrArray &DoctorStartTime,
                    StrArray &DoctorEndTime, StrArray &DocAvailableDays, StrArray &DoctorSpecialization, int &DoctorCount,
                    StrArray &PendingDoctorNames, StrArray &PendingDoctorPasswords, int &PendingDoctorRequestCount,
                    StrArray &Receptionist, StrArray &ReceptionistPassword, StrArray &ReceptionistEmail,
                    StrArray &ReceptionistPhone, StrArray &ReceptionistAddress, int &ReceptionistCount,
                    StrArray &Pharmacist, StrArray &PharmacistPassword, StrArray &PharmacistEmail,
                    StrArray &PharmacistPhone, StrArray &PharmacistAddress, int &PharmacistCount,
                    const StrArray &Medicines, const IntArray &MedicineStock,
                    const IntArray &MedicineReorderLevel, int MedicineCount,
                    const StrArray &appointmentStatus, const StrArray &appointmentDate,
                    const StrArray &appointmentEmergency, int appointmentCount,
                    int patientCount)
{
    int option = 0;
    while (option != 13)
    {
        printHeader();
        
        // Display quick stats
        displayQuickStats(patientCount, appointmentCount, DoctorCount, MedicineCount, appointmentStatus, appointmentCount);
        
        // Check and show low stock alert
        checkLowStockMedicines(Medicines, MedicineStock, MedicineReorderLevel, MedicineCount);
        
        string adminOptions[] = {
            "➕ Add Doctors",
            "➖ Remove Doctors",
            "➕ Add Receptionist",
            "➖ Remove Receptionist",
            "➕ Add Pharmacist",
            "➖ Remove Pharmacist",
            "👀 View Doctors",
            "👀 View Receptionist",
            "👀 View Pharmacist",
            "📋 Review Doctor Signup Requests",
            "⚠️ View Low Stock Medicines",
            "💰 Generate Revenue Report",
            "🚪 Log Out"
        };
        displayMenuBox("🔐  ADMIN CONTROL PANEL  🔐", adminOptions, 13);
        option = SafeIntInput("Enter your choice: ", 1, 13);
        if (option == 1)
        {
            for (int i = DoctorCount; i < UserSize; i++)
            {
                useGreenColour();
                cout << "\n=== Add New Doctor ===" << endl;
                useWhiteColour();
                
                Doctors[i] = PromptLine("Enter the Doctor Name: ");
                DoctorPassword[i] = PromptLine("Enter password for doctor: ");
                DoctorEmail[i] = PromptLine("Enter email address: ");
                DoctorPhone[i] = PromptLine("Enter phone number (e.g., +92-300-1234567): ");
                DoctorAddress[i] = PromptLine("Enter address: ");
                DoctorQualification[i] = PromptLine("Enter qualification (e.g., MBBS, FCPS): ");
                DoctorSpecialization[i] = PromptLine("Enter the Doctor Specialization: ");
                if (DoctorSpecialization[i].empty())
                {
                    DoctorSpecialization[i] = "General Medicine";
                }
                DoctorStartTime[i] = PromptLine("Enter starting time (e.g., 3PM): ");
                DoctorEndTime[i] = PromptLine("Enter ending time (e.g., 5PM): ");
                DocAvailableDays[i] = PromptLine("Enter available days: ");
                DoctorCount++;
                
                // Immediately save to file
                SaveDoctorsToFile(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                                  DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
                
                useGreenColour();
                cout << "✓ Doctor added successfully!" << endl;
                useWhiteColour();
                
                int option2 = SafeIntInput("Press 1 to Enter more doctors, 0 to Exit: ", 0, 1);
                if (option2 == 0)
                {
                    break;
                }
            }
        }
        else if (option == 2)
        {
            string Name;
            Name = PromptLine("Enter the Doctor Name : ");
            for (int i = 0; i < DoctorCount; i++)
            {
                if (Doctors[i] == Name)
                {
                    for (int j = i; j < DoctorCount - 1; j++)
                    {
                        Doctors[j] = Doctors[j + 1];
                        DoctorPassword[j] = DoctorPassword[j + 1];
                        DoctorEmail[j] = DoctorEmail[j + 1];
                        DoctorPhone[j] = DoctorPhone[j + 1];
                        DoctorAddress[j] = DoctorAddress[j + 1];
                        DoctorQualification[j] = DoctorQualification[j + 1];
                        DoctorStartTime[j] = DoctorStartTime[j + 1];
                        DoctorEndTime[j] = DoctorEndTime[j + 1];
                        DocAvailableDays[j] = DocAvailableDays[j + 1];
                        DoctorSpecialization[j] = DoctorSpecialization[j + 1];
                    }
                    DoctorCount--;
                    
                    // Immediately save to file
                    SaveDoctorsToFile(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                                      DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
                    
                    useGreenColour();
                    cout << "✓ Doctor removed successfully!" << endl;
                    useWhiteColour();
                    break;
                }
            }
        }
        else if (option == 3)
        {
            for (int i = ReceptionistCount; i < UserSize; i++)
            {
                useGreenColour();
                cout << "\n=== Add New Receptionist ===" << endl;
                useWhiteColour();
                
                Receptionist[i] = PromptLine("Enter the Receptionist Name: ");
                ReceptionistPassword[i] = PromptLine("Enter password for receptionist: ");
                ReceptionistEmail[i] = PromptLine("Enter email address: ");
                ReceptionistPhone[i] = PromptLine("Enter phone number (e.g., +92-300-1234567): ");
                ReceptionistAddress[i] = PromptLine("Enter address: ");
                ReceptionistCount++;
                
                // Immediately save to file
                SaveReceptionistsToFile(Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount);
                
                useGreenColour();
                cout << "✓ Receptionist added successfully!" << endl;
                useWhiteColour();
                
                int option2 = SafeIntInput("Press 1 for Entering more Receptionists, 0 to Exit: ", 0, 1);
                if (option2 == 0)
                {
                    break;
                }
            }
        }
        else if (option == 4)
        {
            string Name;
            Name = PromptLine("Enter the Receptionist Name : ");
            for (int i = 0; i < ReceptionistCount; i++)
            {
                if (Receptionist[i] == Name)
                {
                    for (int j = i; j < ReceptionistCount - 1; j++)
                    {
                        Receptionist[j] = Receptionist[j + 1];
                        ReceptionistPassword[j] = ReceptionistPassword[j + 1];
                        ReceptionistEmail[j] = ReceptionistEmail[j + 1];
                        ReceptionistPhone[j] = ReceptionistPhone[j + 1];
                        ReceptionistAddress[j] = ReceptionistAddress[j + 1];
                    }
                    ReceptionistCount--;
                    
                    // Immediately save to file
                    SaveReceptionistsToFile(Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount);
                    
                    useGreenColour();
                    cout << "✓ Receptionist removed successfully!" << endl;
                    useWhiteColour();
                    break;
                }
            }
        }
        else if (option == 5)
        {
            for (int i = PharmacistCount; i < UserSize; i++)
            {
                useGreenColour();
                cout << "\n=== Add New Pharmacist ===" << endl;
                useWhiteColour();
                
                Pharmacist[i] = PromptLine("Enter the Pharmacist Name: ");
                PharmacistPassword[i] = PromptLine("Enter password for pharmacist: ");
                PharmacistEmail[i] = PromptLine("Enter email address: ");
                PharmacistPhone[i] = PromptLine("Enter phone number (e.g., +92-300-1234567): ");
                PharmacistAddress[i] = PromptLine("Enter address: ");
                PharmacistCount++;
                
                // Immediately save to file
                SavePharmacistsToFile(Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount);
                
                useGreenColour();
                cout << "✓ Pharmacist added successfully!" << endl;
                useWhiteColour();
                
                int option2 = SafeIntInput("Press 1 for Entering more Pharmacists, 0 to Exit: ", 0, 1);
                if (option2 == 0)
                {
                    break;
                }
            }
        }
        else if (option == 6)
        {
            string Name;
            Name = PromptLine("Enter the Pharmacist Name : ");
            for (int i = 0; i < PharmacistCount; i++)
            {
                if (Pharmacist[i] == Name)
                {
                    for (int j = i; j < PharmacistCount - 1; j++)
                    {
                        Pharmacist[j] = Pharmacist[j + 1];
                        PharmacistPassword[j] = PharmacistPassword[j + 1];
                        PharmacistEmail[j] = PharmacistEmail[j + 1];
                        PharmacistPhone[j] = PharmacistPhone[j + 1];
                        PharmacistAddress[j] = PharmacistAddress[j + 1];
                    }
                    PharmacistCount--;
                    
                    // Immediately save to file
                    SavePharmacistsToFile(Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount);
                    
                    useGreenColour();
                    cout << "✓ Pharmacist removed successfully!" << endl;
                    useWhiteColour();
                    break;
                }
            }
        }
        else if (option == 7)
        {
            ViewDoctors(Doctors, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                       DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
        }
        else if (option == 8)
        {
            ViewReceptionist(Receptionist, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount);
        }
        else if (option == 9)
        {
            ViewPharmacist(Pharmacist, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount);
        }
        else if (option == 10)
        {
            ReviewDoctorSignupRequests(PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                                      Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                                      DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount);
        }
        else if (option == 11)
        {
            // View Low Stock Medicines
            displayLowStockReport(Medicines, MedicineStock, MedicineReorderLevel, MedicineCount);
        }
        else if (option == 12)
        {
            // Generate Revenue Report
            generateRevenueReport(appointmentStatus, appointmentDate, appointmentEmergency, appointmentCount);
        }
        else if (option == 13)
        {
            break;
        }
        clearScreen();
    }
}

// Shows Receptionists menu
void ReceptionInterface(StrArray &appointmentPatient, StrArray &appointmentDoctor, StrArray &appointmentDate,
                        StrArray &appointmentTime, StrArray &appointmentStatus, StrArray &appointmentEmergency,
                        StrArray &PrescriptionMed, StrArray &MedicineTimes, StrArray &PrescriptionDays, int &appointmentCount,
                        StrArray &PatientName, StrArray &PatientPassword, StrArray &PatientAge, int &Patientcount,
                        StrArray &Doctors, StrArray &DoctorStartTime, StrArray &DoctorEndTime, int DoctorCount)
{
    int option = 0;
    while (option != 6)
    {
        printHeader();
        string receptionOptions[] = {
            "📅 Schedule Appointment",
            "➕ Add Patient",
            "🔍 Search Patient",
            "🗑️ Delete Patient",
            "📋 View Appointments",
            "🚪 Log Out"
        };
        displayMenuBox("📝  RECEPTIONIST DASHBOARD  📝", receptionOptions, 6);
        option = SafeIntInput("Enter your choice: ", 1, 6);
        if (option == 1)
        {
            string PTName = PromptLine("Enter Patient Name : ");
            string date = PromptLine("Enter the date for appointment (YYYY-MM-DD) : ");
            string DocName = PromptLine("Enter The Doctor's Name : ");
            if (BookAppointment(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime,
                                 appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                 appointmentCount,
                                 Doctors, DoctorStartTime, DoctorEndTime, DoctorCount,
                                 PTName, DocName, date, false))
            {
                cout << "Appointment Booked Successfully. " << endl;
            }
            else
            {
                cout << " Appointment already exist. " << endl;
            }
        }

        if (option == 2)
        {
            string PTNAME = PromptLine("Enter Patient Name : ");
            string PTAGE = PromptLine(" Enter the Patient Age : ");
            string PP = PromptLine("Enter the Account Password : ");
            if (SignUp(PatientName, PatientPassword, PatientAge, Patientcount, PTNAME, PP, PTAGE))
            {
                cout << "Account Created Successfully. " << endl;
            }
            else
            {
                cout << "Failed to create the account. " << endl;
            }
        }

        if (option == 3)
        {
            string PTName = PromptLine("Enter Patient Name : ");
            if (SearchPatient(appointmentPatient, appointmentCount, PTName))
            {
                showSuccess("Patient Found!");
                useGreenColour();
                cout << "✅ " << PTName << " has appointments in the system" << endl;
                useWhiteColour();
            }
            else
            {
                showError("Patient Not Found.");
            }
        }

        if (option == 4)
        {
            string PTName = PromptLine("Enter Patient Name : ");
            string date = PromptLine("Enter the date for appointment (YYYY-MM-DD) : ");
            if (CancelAppointment(appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime,
                                  appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                  appointmentCount, PTName, date))
            {
                cout << "Appointment Cancelled Successfully. " << endl;
            }
            else
            {
                cout << "Appointment Not Found. " << endl;
            }
        }
        else if (option == 5)
        {
            ViewAppointments(appointmentPatient, appointmentDate, appointmentTime, appointmentDoctor, appointmentCount);
        }
        else if (option == 6)
        {
            break;
        }
        clearScreen();
    }
}

// Shows Pharmacists Menu
void PharmacistInterface(const StrArray &PrescriptionMed, const StrArray &MedicineTimes, const StrArray &PrescriptionDays,
                         int appointmentCount,
                         StrArray &Medicines, StrArray &MedicinePrices, int &MedicineCount)
{
    int option = 0;
    while (option != 3)
    {
        printHeader();
        string pharmacistOptions[] = {
            "📋 View Doctor's Prescription",
            "🏥 Manage Medicine Inventory",
            "🚪 Log Out"
        };
        displayMenuBox("💊  PHARMACIST DASHBOARD  💊", pharmacistOptions, 3);
        option = SafeIntInput("Enter your choice: ", 1, 3);
        if (option == 1)
        {
            ViewPrescriptions(PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount);
        }
        else if (option == 2)
        {
            ManageInventory(Medicines, MedicinePrices, MedicineCount);
        }
        else if (option == 3)
        {
            break;
        }
        clearScreen();
    }
}

// Shows a Login box
void loginScreen()
{
    int width = 50;
    int padding = getCenterPadding(width);
    string pad = string(padding, ' ');
    
    cout << pad;
    drawBox(width);
    cout << pad;
    useGreenColour();
    drawBoxSides("LOGIN CREDENTIALS", width);
    useCyanColour();
    cout << pad << "╚";
    for (int i = 0; i < width - 2; i++)
        cout << "═";
    cout << "╝" << endl;
    useWhiteColour();
    cout << endl;
}

/*____________________Main Code_______________________*/

// Includes all the  main loops  of the program
// Route the users to correct Interface
int main()
{
    // Setup console for UTF-8 and colors
    setupConsole();
    
    StrArray PatientName = {};
    StrArray PatientPassword = {};
    StrArray PatientAge = {};
    int Patientcount = 0;

    StrArray appointmentPatient = {};
    StrArray appointmentDate = {};
    StrArray appointmentDoctor = {};
    StrArray appointmentTime = {};
    StrArray appointmentStatus = {};
    StrArray appointmentEmergency = {};  // Track emergency appointments
    StrArray PrescriptionMed = {};
    StrArray MedicineTimes = {};
    StrArray PrescriptionDays = {};
    int appointmentCount = 0;

    StrArray Doctors = {};
    StrArray DoctorPassword = {};
    StrArray DoctorEmail = {};
    StrArray DoctorPhone = {};
    StrArray DoctorAddress = {};
    StrArray DoctorQualification = {};
    StrArray DoctorStartTime = {};
    StrArray DoctorEndTime = {};
    StrArray DocAvailableDays = {};
    StrArray DoctorSpecialization = {};
    int DoctorCount = 0;

    StrArray PendingDoctorNames = {};
    StrArray PendingDoctorPasswords = {};
    int PendingDoctorRequestCount = 0;

    StrArray Receptionist = {};
    StrArray ReceptionistPassword = {};
    StrArray ReceptionistEmail = {};
    StrArray ReceptionistPhone = {};
    StrArray ReceptionistAddress = {};
    int ReceptionistCount = 0;
    
    StrArray Pharmacist = {};
    StrArray PharmacistPassword = {};
    StrArray PharmacistEmail = {};
    StrArray PharmacistPhone = {};
    StrArray PharmacistAddress = {};
    int PharmacistCount = 0;

    StrArray Medicines = {};
    StrArray MedicinePrices = {};
    IntArray MedicineStock = {};
    IntArray MedicineReorderLevel = {};
    int MedicineCount = 0;

    StrArray HistoryPatients = {};
    StrArray HistoryDetails = {};
    int HistoryCount = 0;

    InitializeDefaults(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                       DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                       Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                       Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                       Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount);

    LoadAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                HistoryPatients, HistoryDetails, HistoryCount);

    int option = 0;

    while (option != 6)
    {
        printHeader();
        option = Menu();
        clearScreen();
        if (option == 1)
        {
            printHeader();
            loginScreen();
            string admin = "admin";
            string AdminPass = PromptLine("Enter Password : ");
            if (AdminPass == "123")
            {
                AdminInterface(Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                               DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                               PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                               Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                               Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                               Medicines, MedicineStock, MedicineReorderLevel, MedicineCount,
                               appointmentStatus, appointmentDate, appointmentEmergency, appointmentCount, Patientcount);
                SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                            appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                            appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                            Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                            DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                            PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                            Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                            Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                            Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                            HistoryPatients, HistoryDetails, HistoryCount);
            }
            else
            {
                cout << "Wrong Credentials " << endl;
            }
        }
        if (option == 2)
        {
            printHeader();
            loginScreen();
            
                DoctorInterface(Doctors, DoctorPassword, DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                                 appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                                 PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                                 HistoryPatients, HistoryDetails, HistoryCount,
                                 PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount);
                SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                            appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                            appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                            Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                            DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                            PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                            Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                            Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                            Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                            HistoryPatients, HistoryDetails, HistoryCount);
        }
            
        
        if (option == 3)
        {
            printHeader();
            loginScreen();
            string Reception = PromptLine("Enter user : ");
            string ReceptionPass = PromptLine("Enter the Password : ");
            if (ReceptionPass == "12345")
            {
                ReceptionInterface(appointmentPatient, appointmentDoctor, appointmentDate,
                                    appointmentTime, appointmentStatus, appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays,
                                    appointmentCount,
                                    PatientName, PatientPassword, PatientAge, Patientcount,
                                    Doctors, DoctorStartTime, DoctorEndTime, DoctorCount);
                SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                            appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                            appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                            Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                            DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                            PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                            Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                            Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                            Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                            HistoryPatients, HistoryDetails, HistoryCount);
            }
            else
            {
                cout << "Wrong Credentials " << endl;
            }
        }
        if (option == 4)
        {
            printHeader();
            loginScreen();
            PatientInterface(PatientName, PatientPassword, PatientAge, Patientcount,
                             appointmentPatient, appointmentDoctor, appointmentDate,
                             appointmentTime, appointmentStatus, appointmentEmergency, 
                             PrescriptionMed, MedicineTimes, PrescriptionDays,
                             appointmentCount,
                             Doctors, DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                             Medicines, MedicinePrices, MedicineCount);
            SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                        appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                        appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                        Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                        DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                        PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                        Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                        Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                        Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                        HistoryPatients, HistoryDetails, HistoryCount);
        }
        if (option == 5)
        {
            printHeader();
            loginScreen();
            string Pharma = PromptLine("Enter User : ");
            string PharmaPass = PromptLine("Enter Password : ");
            if (PharmaPass == "123456")
            {
                PharmacistInterface(PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                                     Medicines, MedicinePrices, MedicineCount);
                SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                            appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                            appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                            Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                            DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                            PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                            Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                            Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                            Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                            HistoryPatients, HistoryDetails, HistoryCount);
            }
            else
            {
                cout << "Wrong Credentials " << endl;
            }
        }

        if (option == 6)
        {
            cout << "Good Bye";
            cout << endl;
            SaveAllData(PatientName, PatientPassword, PatientAge, Patientcount,
                        appointmentPatient, appointmentDoctor, appointmentDate, appointmentTime, appointmentStatus,
                        appointmentEmergency, PrescriptionMed, MedicineTimes, PrescriptionDays, appointmentCount,
                        Doctors, DoctorPassword, DoctorEmail, DoctorPhone, DoctorAddress, DoctorQualification,
                        DoctorStartTime, DoctorEndTime, DocAvailableDays, DoctorSpecialization, DoctorCount,
                        PendingDoctorNames, PendingDoctorPasswords, PendingDoctorRequestCount,
                        Receptionist, ReceptionistPassword, ReceptionistEmail, ReceptionistPhone, ReceptionistAddress, ReceptionistCount,
                        Pharmacist, PharmacistPassword, PharmacistEmail, PharmacistPhone, PharmacistAddress, PharmacistCount,
                        Medicines, MedicinePrices, MedicineStock, MedicineReorderLevel, MedicineCount,
                        HistoryPatients, HistoryDetails, HistoryCount);
        }
        clearScreen();
    }
    
    return 0;
}
