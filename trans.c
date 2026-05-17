// Bank-account program reads a random-access file sequentially,
// updates data already written to the file, creates new data to
// be placed in the file, and deletes data previously in the file.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCOUNTS 100

// clientData structure definition
struct clientData
{
    unsigned int acctNum; // account number
    char lastName[15];    // account last name
    char firstName[10];   // account first name
    double balance;       // account balance
}; // end structure clientData

// prototypes
unsigned int enterChoice(void);
int promptUnsignedInt(const char *prompt, unsigned int *value, unsigned int min, unsigned int max);
int promptDouble(const char *prompt, double *value);
int promptString(const char *prompt, char *dest, size_t size);
int safeFseek(FILE *stream, long offset, int whence);
int safeFread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int safeFwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
void secureZero(void *ptr, size_t size);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void checkBalance(FILE *fPtr);

int main(void)
{
    FILE *cfPtr;          // credit.dat file pointer
    unsigned int choice;  // user's choice

    if ((cfPtr = fopen("credit.dat", "r+b")) == NULL)
    {
        fprintf(stderr, "credit.dat: File could not be opened: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1:
            textFile(cfPtr);
            break;
        case 2:
            updateRecord(cfPtr);
            break;
        case 3:
            newRecord(cfPtr);
            break;
        case 4:
            deleteRecord(cfPtr);
            break;
        case 5:
            checkBalance(cfPtr);
            break;
        default:
            puts("Incorrect choice");
            break;
        }
    }

    if (fclose(cfPtr) != 0)
    {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int readLine(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        return 0;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

int promptUnsignedInt(const char *prompt, unsigned int *value, unsigned int min, unsigned int max)
{
    char buffer[64];
    unsigned long temp;
    char extra;

    while (1)
    {
        printf("%s", prompt);
        if (!readLine(buffer, sizeof(buffer)))
        {
            return 0;
        }

        if (sscanf(buffer, "%lu %c", &temp, &extra) != 1)
        {
            puts("Invalid input; please enter a whole number.");
            continue;
        }

        if (temp < min || temp > max)
        {
            printf("Value must be between %u and %u.\n", min, max);
            continue;
        }

        *value = (unsigned int)temp;
        return 1;
    }
}

int promptDouble(const char *prompt, double *value)
{
    char buffer[64];
    char extra;

    while (1)
    {
        printf("%s", prompt);
        if (!readLine(buffer, sizeof(buffer)))
        {
            return 0;
        }

        if (sscanf(buffer, "%lf %c", value, &extra) != 1)
        {
            puts("Invalid input; please enter a valid number.");
            continue;
        }

        return 1;
    }
}

int promptString(const char *prompt, char *dest, size_t size)
{
    char buffer[128];

    printf("%s", prompt);
    if (!readLine(buffer, sizeof(buffer)))
    {
        return 0;
    }

    if (buffer[0] == '\0')
    {
        puts("Input cannot be empty.");
        return 0;
    }

    strncpy(dest, buffer, size - 1);
    dest[size - 1] = '\0';
    return 1;
}

int safeFseek(FILE *stream, long offset, int whence)
{
    if (fseek(stream, offset, whence) != 0)
    {
        fprintf(stderr, "File seek failed: %s\n", strerror(errno));
        return 0;
    }
    return 1;
}

int safeFread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t readCount = fread(ptr, size, nmemb, stream);
    if (readCount != nmemb && ferror(stream))
    {
        fprintf(stderr, "File read failed: %s\n", strerror(errno));
        return 0;
    }
    return (int)readCount;
}

int safeFwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t writeCount = fwrite(ptr, size, nmemb, stream);
    if (writeCount != nmemb)
    {
        fprintf(stderr, "File write failed: %s\n", strerror(errno));
        return 0;
    }
    if (fflush(stream) != 0)
    {
        fprintf(stderr, "Failed to flush output: %s\n", strerror(errno));
        return 0;
    }
    return 1;
}

void secureZero(void *ptr, size_t size)
{
    volatile unsigned char *p = ptr;
    while (size--)
    {
        *p++ = 0;
    }
}

void textFile(FILE *readPtr)
{
    FILE *writePtr;
    struct clientData client = {0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        fprintf(stderr, "File could not be opened: %s\n", strerror(errno));
        return;
    }

    rewind(readPtr);
    fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");

    while (fread(&client, sizeof(struct clientData), 1, readPtr) == 1)
    {
        if (client.acctNum != 0)
        {
            fprintf(writePtr, "%-6u%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName,
                    client.balance);
        }
    }

    if (ferror(readPtr))
    {
        fprintf(stderr, "Error reading credit file: %s\n", strerror(errno));
    }

    secureZero(&client, sizeof(client));
    fclose(writePtr);
}

void updateRecord(FILE *fPtr)
{
    unsigned int account;
    double transaction;
    struct clientData client = {0};

    if (!promptUnsignedInt("Enter account to update ( 1 - 100 ): ", &account, 1, MAX_ACCOUNTS))
    {
        puts("Failed to read account number.");
        return;
    }

    if (!safeFseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET))
    {
        return;
    }
    if (safeFread(&client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        puts("Unable to read the account record.");
        return;
    }

    if (client.acctNum == 0)
    {
        printf("Account #%u has no information.\n", account);
    }
    else
    {
        printf("%-6u%-16s%-11s%10.2f\n\n", client.acctNum, client.lastName, client.firstName, client.balance);

        if (!promptDouble("Enter charge ( + ) or payment ( - ): ", &transaction))
        {
            puts("Failed to read transaction amount.");
            return;
        }

        client.balance += transaction;
        printf("%-6u%-16s%-11s%10.2f\n", client.acctNum, client.lastName, client.firstName, client.balance);

        if (!safeFseek(fPtr, - (long)sizeof(struct clientData), SEEK_CUR))
        {
            return;
        }
        if (!safeFwrite(&client, sizeof(struct clientData), 1, fPtr))
        {
            puts("Failed to update the account record.");
        }

        secureZero(&client, sizeof(client));
    }
}

void checkBalance(FILE *fPtr)
{
    unsigned int account;
    struct clientData client = {0};

    if (!promptUnsignedInt("Enter account to check balance ( 1 - 100 ): ", &account, 1, MAX_ACCOUNTS))
    {
        puts("Failed to read account number.");
        return;
    }

    if (!safeFseek(fPtr, (long)(account - 1) * sizeof(struct clientData), SEEK_SET))
    {
        return;
    }
    if (safeFread(&client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        puts("Unable to read the account record.");
        return;
    }

    if (client.acctNum == 0)
    {
        printf("Account #%u has no information.\n", account);
    }
    else
    {
        printf("Account #%u: %s %s\n", client.acctNum, client.firstName, client.lastName);
        printf("Current balance: %10.2f\n", client.balance);
        if (client.balance < 0.0)
        {
            puts("Warning: account balance is negative.");
        }
    }

    secureZero(&client, sizeof(client));
}

void deleteRecord(FILE *fPtr)
{
    struct clientData client;
    struct clientData blankClient = {0};
    unsigned int accountNum;

    if (!promptUnsignedInt("Enter account number to delete ( 1 - 100 ): ", &accountNum, 1, MAX_ACCOUNTS))
    {
        puts("Failed to read account number.");
        return;
    }

    if (!safeFseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET))
    {
        return;
    }
    if (safeFread(&client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        puts("Unable to read the account record.");
        return;
    }

    if (client.acctNum == 0)
    {
        printf("Account %u does not exist.\n", accountNum);
    }
    else
    {
        if (!safeFseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET))
        {
            return;
        }
        if (!safeFwrite(&blankClient, sizeof(struct clientData), 1, fPtr))
        {
            puts("Failed to delete the account record.");
        }
        secureZero(&client, sizeof(client));
    }
}

void newRecord(FILE *fPtr)
{
    struct clientData client = {0};
    unsigned int accountNum;

    if (!promptUnsignedInt("Enter new account number ( 1 - 100 ): ", &accountNum, 1, MAX_ACCOUNTS))
    {
        puts("Failed to read account number.");
        return;
    }

    if (!safeFseek(fPtr, (long)(accountNum - 1) * sizeof(struct clientData), SEEK_SET))
    {
        return;
    }
    if (safeFread(&client, sizeof(struct clientData), 1, fPtr) != 1)
    {
        puts("Unable to read the account record.");
        return;
    }

    if (client.acctNum != 0)
    {
        printf("Account #%u already contains information.\n", client.acctNum);
    }
    else
    {
        while (!promptString("Enter lastname: ", client.lastName, sizeof(client.lastName)))
        {
            ;
        }
        while (!promptString("Enter firstname: ", client.firstName, sizeof(client.firstName)))
        {
            ;
        }
        if (!promptDouble("Enter balance: ", &client.balance))
        {
            puts("Failed to read balance.");
            secureZero(&client, sizeof(client));
            return;
        }

        client.acctNum = accountNum;
        if (!safeFseek(fPtr, (long)(client.acctNum - 1) * sizeof(struct clientData), SEEK_SET))
        {
            secureZero(&client, sizeof(client));
            return;
        }
        if (!safeFwrite(&client, sizeof(struct clientData), 1, fPtr))
        {
            puts("Failed to write the new account record.");
        }
        secureZero(&client, sizeof(client));
    }
}

unsigned int enterChoice(void)
{
    unsigned int menuChoice = 0;

    puts("\nEnter your choice");
    puts("1 - store a formatted text file of accounts called");
    puts("    \"accounts.txt\" for printing");
    puts("2 - update an account");
    puts("3 - add a new account");
    puts("4 - delete an account");
    puts("5 - check an account balance");
    puts("6 - end program");

    if (!promptUnsignedInt("? ", &menuChoice, 1, 6))
    {
        puts("Invalid menu choice.");
    }

    return menuChoice;
} // end function enterChoice