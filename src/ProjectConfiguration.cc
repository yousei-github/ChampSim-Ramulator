#include "champsim.h"

OutputMemoryTraceFileType outputmemorytrace_one;

OutputChampSimStatisticsFileType outputchampsimstatistics;

void output_memory_trace_initialization(const char* string)
{
    char* string2 = (char*)malloc(strlen(string) + 1);
    strcpy(string2, string);

    const char* delimiter = "/";
    char* token;
    char* last_token;

    /* get the first token */
    token = strtok(string2, delimiter);

    /* walk through other tokens */
    while (token != NULL)
    {
        last_token = token;
        token = strtok(NULL, delimiter);
    }
    
    // append file_extension to string3.
    const char* file_extension = ".trace";
    char* string3 = (char*)malloc(strlen(last_token) + 1 + strlen(file_extension));
    strcpy(string3, last_token);
    strcat(string3, file_extension);

    outputmemorytrace_one.trace_file = fopen(string3, "w");
    outputmemorytrace_one.trace_string = (char*)malloc(strlen(string3) + 1);
    strcpy(outputmemorytrace_one.trace_string, string3);
}

void output_memory_trace_deinitialization(OutputMemoryTraceFileType& outputmemorytrace)
{
    fclose(outputmemorytrace.trace_file);
    printf("Output memory trace into %s.\n", outputmemorytrace.trace_string);

    free(outputmemorytrace.trace_string);
}

void output_memory_trace_hexadecimal(OutputMemoryTraceFileType& outputmemorytrace, uint64_t address, char type)
{
    fprintf(outputmemorytrace.trace_file, "0x%lx %c\n", address, type);
}

void output_champsim_statistics_initialization(const char* string)
{
    char* string2 = (char*)malloc(strlen(string) + 1);
    strcpy(string2, string);

    const char* delimiter = "/";
    char* token;
    char* last_token;

    /* get the first token */
    token = strtok(string2, delimiter);

    /* walk through other tokens */
    while (token != NULL)
    {
        last_token = token;
        token = strtok(NULL, delimiter);
    }
    
    // append file_extension to string3.
    const char* file_extension = ".statistics";
    char* string3 = (char*)malloc(strlen(last_token) + 1 + strlen(file_extension));
    strcpy(string3, last_token);
    strcat(string3, file_extension);

    outputchampsimstatistics.trace_file = fopen(string3, "w");
    outputchampsimstatistics.trace_string = (char*)malloc(strlen(string3) + 1);
    strcpy(outputchampsimstatistics.trace_string, string3);
}

void output_champsim_statistics_deinitialization(OutputChampSimStatisticsFileType& outputchampsimstatistics)
{
    fclose(outputchampsimstatistics.trace_file);
    printf("Output ChampSim statistics into %s.\n", outputchampsimstatistics.trace_string);

    free(outputchampsimstatistics.trace_string);
}
