#define _WIN32_WINNT 0x501
#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _AFXDLL //<<===notice this

#include <Afxwin.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <conio.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

#include <openssl/evp.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 1024
#pragma warning(disable : 4996)
using namespace std;

UINT  ServerThread(LPVOID pParam);

int encrypt(const unsigned char* plaintext, int plaintext_len, unsigned char* key,
    unsigned char* iv, unsigned char* ciphertext);
int decrypt(unsigned char* ciphertext, int ciphertext_len, unsigned char* key,
    unsigned char* iv, unsigned char* plaintext);
void decryptDatabase(vector<string>& database, string encryptionPassword);
bool modifyDatabase(string command, vector<string>& database);
void encryptDatabase(vector<string>& database, string encryptionPassword);
void sendDatabase(vector<string> database, SOCKET client);
string requestDatabasePassword(bool isNewDB, SOCKET client);

#include <Windows.h>
#include <tchar.h>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  "Database app server service"

/*
Program entry point
*/
int _tmain(int argc, TCHAR* argv[])
{
    cout << "entered main\n";
    ofstream test("log");
    test << "I AM ALIVE";
    test.close();
    OutputDebugString(_T("Database service: Main: Entry"));
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(LPWSTR)_T(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        cout << "Database service: Main: StartServiceCtrlDispatcher returned error\n";
        OutputDebugString(_T("Database service: Main: StartServiceCtrlDispatcher returned error"));
        return GetLastError();
    }

    OutputDebugString(_T("Database service: Main: Exit"));
    return 0;
}


/*
Windows service entry point
*/
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    cout << "entered service main\n";
    DWORD Status = E_FAIL;

    OutputDebugString(_T("Database service: ServiceMain: Entry"));

    g_StatusHandle = RegisterServiceCtrlHandler((LPWSTR)_T(SERVICE_NAME), ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        OutputDebugString(_T("Database service: ServiceMain: RegisterServiceCtrlHandler returned error"));
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("Database service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks neccesary to start the service here
     */
    OutputDebugString(_T("Database service: ServiceMain: Performing Service Start Operations"));

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        OutputDebugString(_T("Database service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("Database service: ServiceMain: SetServiceStatus returned error"));
        }
        OutputDebugString(_T("Database service: ServiceMain: Exit"));
        return;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("Database service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start the thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    OutputDebugString(_T("Database service: ServiceMain: Waiting for Worker Thread to complete"));

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);

    OutputDebugString(_T("Database service: ServiceMain: Worker Thread Stop Event signaled"));


    /*
     * Perform any cleanup tasks
     */
    OutputDebugString(_T("Database service: ServiceMain: Performing Cleanup Operations"));

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("Database service: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    OutputDebugString(_T("Database service: ServiceMain: Exit"));

    return;
}


/*
Service control handler
*/
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    OutputDebugString(_T("Database service: ServiceCtrlHandler: Entry"));

    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        OutputDebugString(_T("Database service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks neccesary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("Database service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }

    OutputDebugString(_T("Database service: ServiceCtrlHandler: Exit"));
}


/*
Service main thread
*/
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    cout << "entered serviceworkerthread";
    OutputDebugString(_T("Database service: ServiceWorkerThread: Entry"));

    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        CWinThread* singleUserThread = AfxBeginThread(ServerThread, 0);
        WaitForSingleObject(singleUserThread, INFINITE);
    }

    OutputDebugString(_T("Database service: ServiceWorkerThread: Exit"));

    return ERROR_SUCCESS;
}


/*
Thread running the server
*/
UINT  ServerThread(LPVOID pParam)
{
    SOCKET server;

    WSADATA wsaData;
    sockaddr_in local;
    int wsaret = WSAStartup(0x101, &wsaData);

    if (wsaret != 0)
        return 0;

    local.sin_family = AF_INET; 
    local.sin_addr.s_addr = INADDR_ANY; 
    local.sin_port = htons((u_short)20248); 

    server = socket(AF_INET, SOCK_STREAM, 0);

    if (server == INVALID_SOCKET)
        return 0;

    if (bind(server, (sockaddr*)&local, sizeof(local)) != 0)
        return 0;

    if (listen(server, 10) != 0)
        return 0;
  
    SOCKET client;
    sockaddr_in from;
    int fromlen = sizeof(from);

    // Establishing connection
    client = accept(server,
        (struct sockaddr*)&from, &fromlen);

    OutputDebugString(_T("Database service: User connected"));

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    fstream database_read("database");
    // If database file is empty - nothing to decrypt - new password is needed
    bool isNewDB = database_read.peek() == std::ifstream::traits_type::eof();

    database_read.close();

    string encryptionPassword = requestDatabasePassword(isNewDB, client);
    if (encryptionPassword == "") {
        return 1;
    }

    vector <string> database;
    string line;
    while (getline(database_read, line)) {
        database.push_back(line);
    }
    if (!isNewDB) {
        decryptDatabase(database, encryptionPassword);
        sendDatabase(database, client);
    }

    // main working cycle
    do {
        // empty the reception buffer
        for (int i = 0; i < recvbuflen; i++) {
            recvbuf[i] = 0;
        }
        iResult = recv(client, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            string receivedMessage = recvbuf;
            // this sycle is in case many messages end up in the same buffer
            cout << receivedMessage << "\n";
            while (receivedMessage != "") {
                if (modifyDatabase(receivedMessage.substr(0, receivedMessage.find('/')), database)) {
                    closesocket(client);
                    WSACleanup();
                    break;
                }
                if (std::count(receivedMessage.begin(), receivedMessage.end(), '/') == 1) break;
                receivedMessage = receivedMessage.substr(receivedMessage.find('/') + 1, receivedMessage.size() - 1 - receivedMessage.find('/'));
            }
        }
        else if (iResult == 0)
            OutputDebugString(_T("Database service: Connection closing"));
        else {
            string errorstr = "Database service: recv failed with error: " + to_string(WSAGetLastError());
            OutputDebugString((LPWSTR)(errorstr.c_str()));
            closesocket(client);
            WSACleanup();
            break;
        }
    } while (iResult > 0);

    encryptDatabase(database, encryptionPassword);
    closesocket(server);
    WSACleanup();

    return 0;
}

// returns user's password for encryption
string requestDatabasePassword(bool isNewDB, SOCKET client) {
    string encryptionPassword;

    string passwordRequestMessage;
    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    if (isNewDB) {
        OutputDebugString(_T("Database service: Database did not exist"));
        passwordRequestMessage = "No previous data. Provide password to encrypt future data.";
    }
    else {
        OutputDebugString(_T("Database service: Database existed"));
        passwordRequestMessage = "Provide password to decrypt existing data.";
    }
    strcpy(recvbuf, passwordRequestMessage.c_str());

    iSendResult = send(client, recvbuf, passwordRequestMessage.size(), 0);
    if (iSendResult == SOCKET_ERROR) {
        OutputDebugString(_T("Database service: Send failed with error"));
        closesocket(client);
        WSACleanup();
        return "";
    }

    char passbuf[DEFAULT_BUFLEN];
    for (int i = 0; i < recvbuflen; i++) {
        passbuf[i] = 0;
    }
    int iResult = recv(client, passbuf, recvbuflen, 0);
    for (int i = 0; i < recvbuflen; i++) {
        if (passbuf[i] == 0) break;
        encryptionPassword += passbuf[i];
    }
    return encryptionPassword;
}

// sends data in database
void sendDatabase(vector<string> database, SOCKET client) {
    string message = "";
    message += to_string(database.size()) + "/";
    for (int i = 0; i < database.size(); i++) {
        message += database[i] + "/";
    }

    char recvbuf[DEFAULT_BUFLEN];
    strcpy(recvbuf, message.c_str());

    int iSendResult = send(client, recvbuf, message.size(), 0);
}

// encrypt database and write it to file
void encryptDatabase(vector<string>& database, string encryptionPassword) {
     // A 256 bit key 
    unsigned char* key = new unsigned char[32];
    for (int i = 0; i < 32; i++) {
        key[i] = 0;
    }
    for (int i = 0; i < min(encryptionPassword.size(), 32); i++) {
        key[i] = encryptionPassword[i];
    }

    string databaseText = "";
    for (int i = 0; i < database.size(); i++) {
        databaseText += database[i] + '/';
    }
    // A 128 bit IV 
    unsigned char* iv = (unsigned char*)"0123456789012345";

    // Message to be encrypted 
    unsigned char* plaintext = new unsigned char[databaseText.size()];
    for (int i = 0; i < databaseText.size(); i++) {
        plaintext[i] = (unsigned char)databaseText[i];
    }

    /*
     * Buffer for ciphertext. Ensure the buffer is long enough for the
     * ciphertext which may be longer than the plaintext, depending on the
     * algorithm and mode.
     */
    unsigned char ciphertext[2048];

    // Buffer for the decrypted text 
    unsigned char decryptedtext[2048];

    int decryptedtext_len, ciphertext_len;

    // Encrypt the plaintext 
    ciphertext_len = encrypt(plaintext, databaseText.size(), key, iv,
        ciphertext);

    // Do something useful with the ciphertext here 
    ofstream database_write("database");
    for (int i = 0; i < ciphertext_len; i++) {
        database_write << ciphertext[i];
        cout << ciphertext[i];
    }
    database_write.close();
}

// decyphers database from file using password
void decryptDatabase(vector<string>& database, string encryptionPassword) {
    unsigned char ciphertext[2048];

    ifstream database_read("database");
    stringstream buffer;
    buffer << database_read.rdbuf();

    unsigned char* key = new unsigned char[32];
    for (int i = 0; i < 32; i++) {
        key[i] = 0;
    }
    for (int i = 0; i < min(encryptionPassword.size(), 32); i++) {
        key[i] = encryptionPassword[i];
    }
    unsigned char* iv = (unsigned char*)"0123456789012345";
    unsigned char decryptedtext[2048];
    for (int i = 0; i < buffer.str().size(); i++) {
        ciphertext[i] = buffer.str()[i];
    }
    int decryptedtext_len = decrypt(ciphertext, buffer.str().size(), key, iv,
        decryptedtext);

    /* Add a NULL terminator. We are expecting print able text */
    string substr = "";
    for (int i = 0; i < decryptedtext_len; i++) {
        if (decryptedtext[i] == '/') {
            database.push_back(substr);
            substr = "";
        }
        else {
            substr += decryptedtext[i];
        }
    }
    decryptedtext[decryptedtext_len] = '\0';
}

// encrypts string into string
int encrypt(const unsigned char* plaintext, int plaintext_len, unsigned char* key,
    unsigned char* iv, unsigned char* ciphertext)
{
    EVP_CIPHER_CTX* ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        printf("Error with openssl has occured!");

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        printf("Error with openssl has occured!");

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        printf("Error with openssl has occured!");
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        printf("Error with openssl has occured!");
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// decrypts from string using password
int decrypt(unsigned char* ciphertext, int ciphertext_len, unsigned char* key,
    unsigned char* iv, unsigned char* plaintext)
{
    EVP_CIPHER_CTX* ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        OutputDebugString(_T("Database service: error with openssl has occured!"));

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        OutputDebugString(_T("Database service: error with openssl has occured!"));

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        OutputDebugString(_T("Database service: error with openssl has occured!"));
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        OutputDebugString(_T("Database service: error with openssl has occured!"));
    plaintext_len += len; 

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

// modifies database according to command
bool modifyDatabase(string command, vector<string>& database) {
    string value = command.substr(command.find('|') + 1, command.size() - command.find('|') - 1);
    command = command.substr(0, command.find('|'));
    if (command == "New") {
        database.push_back(value);
    }
    if (command == "Delete") {
        int index = stoi(value);
        database.erase(std::next(database.begin(), index));
    }
    if (command == "Edit") {
        int index = stoi(value.substr(0, value.find('|')));
        value = value.substr(value.find('|') + 1, value.size() - value.find('|') - 1);
        database[index] = value;
    }
    if (command == "End") {
        return true;
    }
    return false;
}
