#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_TRAINS 100
#define TUNNEL_LENGTH 100
#define TRAIN_SPEED 100
double SIMULATION_TIME = 60;
double PROBABILITY_P = 0.5;
int total_trains_outside = 0;
int allow_train_arrival = 1; // 1 indicates arrivals are allowed, 0 indicates arrivals are suspended

typedef struct
{
    int id;
    char start_point;
    char end_point;
    int length;
    char arrival_time[9];
    time_t departure_time;
    int hasLeftSystem;
} Train;

// Global variables
int train_counter = 0;
Train trains[MAX_TRAINS];
pthread_mutex_t lock;
pthread_cond_t condition_var;

// prototypes
void *train_arrival(void *args);
void *tunnel_control(void *args);
void initialize_simulation();
void log_train(Train *t);
void log_event(const char *event, Train t);
void getCurrentTime();
void printWaitingQueue();
Train createTrainWithArrival_A();
Train createTrainWithArrival_B();
Train createTrainWithArrival_E();
Train createTrainWithArrival_F();

// Log files
FILE *train_log_file;
FILE *control_log_file;

int main(int argc, char *argv[])
{

    if (argc == 2)
    {
        PROBABILITY_P = atof(argv[1]);
    }
    else if (argc == 4)
    {
        PROBABILITY_P = atof(argv[1]);
        SIMULATION_TIME = atoi(argv[3]);
    }
    else if (argc == 1)
    {
        // leave PROBABILITY_P and SIMULATION_TIME as is
    }
    else
    {
        printf("Usage: ./simulation <creation_probability> <-s simulation_time>");
        return 0;
    }

    srand((unsigned int)time(NULL)); // Seed for randomness

    initialize_simulation();

    pthread_t train_thread, control_thread;

    // threads creation
    pthread_create(&train_thread, NULL, train_arrival, NULL);
    pthread_create(&control_thread, NULL, tunnel_control, NULL);

    // Waiting for threads to finish
    pthread_join(train_thread, NULL);
    pthread_join(control_thread, NULL);

    // Clean up
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&condition_var);
    fclose(train_log_file);
    fclose(control_log_file);

    return 0;
}

void *train_arrival(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&lock);
        if (total_trains_outside > 10)
        {
            // trains slow down
            char timeBuffer[9]; // HH:MM:SS\0
            getCurrentTime(timeBuffer);
            fprintf(control_log_file, "System overload at %s - Trains waiting...\n", timeBuffer);
            fflush(control_log_file);
            allow_train_arrival = 0; // Suspend new train arrivals
        }
        if (allow_train_arrival && train_counter < MAX_TRAINS)
        {
            srand(time(NULL));
            // Generate a random number between 0 and RAND_MAX
            int randomValue = rand();

            // Convert the random number to a value between 0 and 1
            double randomFraction = (double)randomValue / RAND_MAX;

            if (randomFraction < PROBABILITY_P)
            {
                Train t = createTrainWithArrival_A();
                total_trains_outside++;
                trains[train_counter++] = t;
                log_train(&t);
            }

            randomValue = rand();

            // Convert the random number to a value between 0 and 1
            randomFraction = (double)randomValue / RAND_MAX;
            if (randomFraction < PROBABILITY_P)
            {
                Train t = createTrainWithArrival_E();
                total_trains_outside++;
                trains[train_counter++] = t;
                log_train(&t);
            }

            randomValue = rand();

            // Convert the random number to a value between 0 and 1
            randomFraction = (double)randomValue / RAND_MAX;
            if (randomFraction < PROBABILITY_P)
            {
                Train t = createTrainWithArrival_F();
                total_trains_outside++;
                trains[train_counter++] = t;
                log_train(&t);
            }

            randomValue = rand();

            // Convert the random number to a value between 0 and 1
            randomFraction = (double)randomValue / RAND_MAX;
            if (randomFraction > PROBABILITY_P)
            {
                Train t = createTrainWithArrival_B();
                total_trains_outside++;
                trains[train_counter++] = t;
                log_train(&t);
            }
        }
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
    return NULL;
}

void *tunnel_control(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&lock);
        int all_trains_departed = 1;
        for (int i = 0; i < train_counter; i++)
        {
            if (trains[i].departure_time == 0)
            { // Train is waiting to depart

                int extraTime = 0;
                // Seed the random number generator with the current time
                srand(time(NULL));

                // Generate a random number between 0 and RAND_MAX
                int randomInt = rand();

                // Map the random number to a floating-point value between 0 and 1
                double randomDouble = (double)randomInt / RAND_MAX;

                if (randomDouble < 0.1)
                {
                    log_event("Breakdown", trains[i]);
                    extraTime = 4;
                }
                sleep((TUNNEL_LENGTH / TRAIN_SPEED) + extraTime);

                trains[i].departure_time = time(NULL);
                trains[i].hasLeftSystem = 1;
                log_event("Tunnel passing", trains[i]);

                total_trains_outside--;
                break; // Let only one train pass at a time
            }
            else
            {
                all_trains_departed = 0;
            }
        }

        if (all_trains_departed)
        {
            allow_train_arrival = 1;
        }

        pthread_mutex_unlock(&lock);
        sleep(1);
    }
    return NULL;
}

void initialize_simulation()
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&condition_var, NULL);

    train_log_file = fopen("train.log", "w");
    control_log_file = fopen("control.log", "w");

    if (!train_log_file || !control_log_file)
    {
        perror("Error opening log files");
        exit(1);
    }
}

void log_train(Train *t)
{
    fprintf(train_log_file, "Train ID: %d, Start: %c, End: %c, Length: %dm, Arrival: %s\n",
            t->id, t->start_point, t->end_point, t->length, t->arrival_time);
    fflush(train_log_file);
}

void log_event(const char *event, Train t)
{
    char timeBuffer[9]; // HH:MM:SS\0
    getCurrentTime(timeBuffer);
    fprintf(control_log_file, "ID: %d --- %s at %s || ", t.id, event, timeBuffer);
    fflush(control_log_file);
    printWaitingQueue();
}

void printWaitingQueue()
{
    fprintf(control_log_file, "QUEUE: ");
    for (int i = 0; i < train_counter; ++i)
    {
        if (!trains[i].hasLeftSystem)
        {
            fprintf(control_log_file, "%d, ", trains[i].id);
        }
    }
    fprintf(control_log_file, "\n");
    fflush(control_log_file);
}

void getCurrentTime(char *timeString)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(timeString, 9, "%H:%M:%S", timeinfo);
}

Train createTrainWithArrival_A()
{
    Train t;
    t.hasLeftSystem = 0;
    t.id = train_counter;
    t.start_point = 'A';
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    // Generate a random number (0 or 1)
    int randomNum = rand() % 2;

    // Assign 'E' if randomNum is 0, otherwise assign 'F'
    t.end_point = (randomNum == 0) ? 'E' : 'F';

    // Print the result
    // printf("Randomly assigned char: %c\n", result);

    t.length = ((double)rand() / (double)RAND_MAX) > 0.7 ? 100 : 200;
    char timeBuffer[9]; // HH:MM:SS\0
    getCurrentTime(timeBuffer);
    strcpy(t.arrival_time, timeBuffer);

    t.departure_time = 0; // not yet departed
    return t;
}

Train createTrainWithArrival_B()
{
    Train t;
    t.hasLeftSystem = 0;
    t.id = train_counter;
    t.start_point = 'B';
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    // Generate a random number (0 or 1)
    int randomNum = rand() % 2;

    // Assign 'E' if randomNum is 0, otherwise assign 'F'
    t.end_point = (randomNum == 0) ? 'E' : 'F';

    // Print the result
    // printf("Randomly assigned char: %c\n", result);

    t.length = ((double)rand() / (double)RAND_MAX) > 0.7 ? 100 : 200;
    char timeBuffer[9]; // HH:MM:SS\0
    getCurrentTime(timeBuffer);
    strcpy(t.arrival_time, timeBuffer);

    t.departure_time = 0; // not yet departed
    return t;
}

Train createTrainWithArrival_E()
{
    Train t;
    t.hasLeftSystem = 0;
    t.id = train_counter;
    t.start_point = 'E';
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    // Generate a random number (0 or 1)
    int randomNum = rand() % 2;

    // Assign 'E' if randomNum is 0, otherwise assign 'F'
    t.end_point = (randomNum == 0) ? 'A' : 'B';

    // Print the result
    // printf("Randomly assigned char: %c\n", result);

    t.length = ((double)rand() / (double)RAND_MAX) > 0.7 ? 100 : 200;
    char timeBuffer[9]; // HH:MM:SS\0
    getCurrentTime(timeBuffer);
    strcpy(t.arrival_time, timeBuffer);

    t.departure_time = 0; // not yet departed
    return t;
}

Train createTrainWithArrival_F()
{
    Train t;
    t.hasLeftSystem = 0;
    t.id = train_counter;
    t.start_point = 'F';
    // Seed the random number generator with the current time
    srand((unsigned int)time(NULL));

    // Generate a random number (0 or 1)
    int randomNum = rand() % 2;

    // Assign 'E' if randomNum is 0, otherwise assign 'F'
    t.end_point = (randomNum == 0) ? 'A' : 'B';

    // Print the result
    // printf("Randomly assigned char: %c\n", result);

    t.length = ((double)rand() / (double)RAND_MAX) > 0.7 ? 100 : 200;
    char timeBuffer[9]; // HH:MM:SS\0
    getCurrentTime(timeBuffer);
    strcpy(t.arrival_time, timeBuffer);

    t.departure_time = 0; // not yet departed
    return t;
}
