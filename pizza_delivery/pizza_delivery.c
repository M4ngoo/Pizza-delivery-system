#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "p3190027-p3190221-pizza.h"

int random_num(int lower, int upper)
{
    int number = (rand() % (upper - lower + 1)) + lower;
}

char* get_time()
{
    time_t systime = time(NULL);
    char *str_time = ctime(&systime);
    str_time[strlen(str_time) - 1] = '\0';
    return str_time;
}

//-------info-------
int operators = 3;
int cookers = 2;
int ovens = 10;
int delivery = 7;
int packer = 1;
int pizza_cost = 10;
int first_counter = 0;

//------statistics------
int earnings = 0;
int failed_orders = 0;
int successful_orders = 0;
int avg_wait_time = 0;
int max_wait_time = 0;
int avg_cooling_time = 0;
int max_cooling_time = 0;
int avg_service_time = 0;
int max_service_time = 0;

//-----------------core----------------
pthread_mutex_t mutex_wait_for_op;
pthread_cond_t cond_op;
pthread_mutex_t mutex_release_op;
pthread_mutex_t mutex_earnigs_cal;
pthread_mutex_t mutex_wait_for_cook;
pthread_cond_t cond_cook;
pthread_mutex_t mutex_wait_for_ovens;
pthread_cond_t cond_oven;
pthread_mutex_t mutex_release_cooker;
pthread_mutex_t mutex_release_ovens;
pthread_mutex_t mutex_wait_for_packer;
pthread_cond_t cond_packer;
pthread_mutex_t mutex_release_packer;
pthread_mutex_t mutex_wait_for_delivery;
pthread_cond_t cond_delivery;
pthread_mutex_t mutex_release_delivery;

//-----------------additional------------
pthread_mutex_t mutex_wait_time_cal;
pthread_mutex_t mutex_cooling_service_time_cal;
pthread_mutex_t mutex_first_thread;


void *handle_clients(void *orderid)
{
    pthread_mutex_lock(&mutex_first_thread);
    if(first_counter > 0){sleep(random_num(1, 5));}
    else{first_counter += 1;}
    pthread_mutex_unlock(&mutex_first_thread);

    int *order_id = (int *) orderid;
    int wait_time_before = time(NULL);
    pthread_mutex_lock(&mutex_wait_for_op);
    while(operators == 0)
    {
        printf("No available operator waiting... from order : %d\n", *order_id);
        pthread_cond_wait(&cond_op, &mutex_wait_for_op);
    }

    operators -= 1;
    int wait_time_after = time(NULL);

    pthread_mutex_lock(&mutex_wait_time_cal);
    avg_wait_time += wait_time_after - wait_time_before;
    if ((wait_time_after - wait_time_before) > max_wait_time){max_wait_time = wait_time_after - wait_time_before;}
    pthread_mutex_unlock(&mutex_wait_time_cal);

    pthread_mutex_unlock(&mutex_wait_for_op);
    int prep_time_before = time(NULL);
    int pizza_count = random_num(1, 5);
    
    sleep(random_num(1, 2));

    int is_failed = random_num(1, 100);

    if (is_failed == 1 || is_failed == 2 || is_failed == 3 || is_failed == 4 || is_failed == 5)
    {
        char *mytime = get_time();  
        printf("The order with the id %d failed! at %s \n", *order_id, mytime);

        pthread_mutex_lock(&mutex_release_op);
        failed_orders += 1;
        operators += 1;
        pthread_cond_signal(&cond_op);
        pthread_mutex_unlock(&mutex_release_op);

        pthread_exit(order_id);
    }
    else
    {
        char *mytime = get_time(); 
        printf("The order with the id %d has been registered at %s \n", *order_id, mytime);

        pthread_mutex_lock(&mutex_release_op);
        operators += 1;
        pthread_cond_signal(&cond_op);
        pthread_mutex_unlock(&mutex_release_op);

        pthread_mutex_lock(&mutex_earnigs_cal);
        earnings = earnings + (pizza_cost * pizza_count);
        pthread_mutex_unlock(&mutex_earnigs_cal);

        pthread_mutex_lock(&mutex_wait_for_cook);
        while(cookers == 0)
        {
            printf("No available cooker waiting... from order : %d\n", *order_id);
            pthread_cond_wait(&cond_cook, &mutex_wait_for_cook);
        }

        cookers -= 1;
        pthread_mutex_unlock(&mutex_wait_for_cook);

        sleep(pizza_count);

        pthread_mutex_lock(&mutex_wait_for_ovens);
        while(ovens < pizza_count)
        {
            printf("Not enough ovens waiting... from order : %d\n", *order_id);
            pthread_cond_wait(&cond_oven, &mutex_wait_for_ovens);
        }
        
        ovens -= pizza_count;
        pthread_mutex_unlock(&mutex_wait_for_ovens);

        pthread_mutex_lock(&mutex_release_cooker);
        cookers += 1;
        pthread_cond_signal(&cond_cook);
        pthread_mutex_unlock(&mutex_release_cooker);

        sleep(10);
        int cooling_time_before = time(NULL);

        pthread_mutex_lock(&mutex_wait_for_packer);
        while(packer == 0)
        {
            printf("No available packer waiting... from order : %d\n", *order_id);
            pthread_cond_wait(&cond_packer, &mutex_wait_for_packer);
        }

        packer -= 1;
        pthread_mutex_unlock(&mutex_wait_for_packer);

        sleep(2);
        int prep_time_after = time(NULL);
        printf("The preperation of the order %d finished in %d minutes\n", *order_id, prep_time_after - prep_time_before);
        pthread_mutex_lock(&mutex_release_ovens);
        ovens += pizza_count;
        pthread_cond_signal(&cond_oven);
        pthread_mutex_unlock(&mutex_release_ovens);

        pthread_mutex_lock(&mutex_release_packer);
        packer += 1;
        pthread_cond_signal(&cond_packer);
        pthread_mutex_unlock(&mutex_release_packer);

        pthread_mutex_lock(&mutex_wait_for_delivery);
        while(delivery == 0)
        {
            printf("No available delivery waiting... from order : %d\n", *order_id);
            pthread_cond_wait(&cond_delivery, &mutex_wait_for_delivery);
        }

        delivery -= 1;
        pthread_mutex_unlock(&mutex_wait_for_delivery);

        int time_before_del = time(NULL);
        sleep(random_num(5, 15));
        int time_after_del = time(NULL);
        printf("The delivery of the order %d finished in %d minutes\n", *order_id, time_after_del - prep_time_before);

        pthread_mutex_lock(&mutex_cooling_service_time_cal);
        avg_cooling_time += time_after_del - cooling_time_before;
        if((time_after_del - cooling_time_before) > max_cooling_time){max_cooling_time = time_after_del - cooling_time_before;}
        avg_service_time += time_after_del - wait_time_before;
        if((time_after_del - wait_time_before) > max_service_time){max_service_time = time_after_del - wait_time_before;}
        pthread_mutex_unlock(&mutex_cooling_service_time_cal);

        sleep(time_after_del - time_before_del);

        pthread_mutex_lock(&mutex_release_delivery);
        delivery += 1;
        successful_orders += 1;
        pthread_cond_signal(&cond_delivery);
        pthread_mutex_unlock(&mutex_release_delivery);   
    }
    pthread_exit(order_id);
}


int main(int argc, char * argv[])
{

    if (argc != 3)
    {
        printf("ERROR: the number of arguements is invalid!");
    }

    int numberOfCustomers = atoi(argv[1]);
    int seed = atoi(argv[2]);
    srand(seed);

    int orderId[numberOfCustomers];
    pthread_t orders[numberOfCustomers];

    void *status;
    
    for (int counter = 0; counter < numberOfCustomers; counter++)
    {
        orderId[counter] = counter + 1;
        
        pthread_create(&orders[counter], NULL, handle_clients, &orderId[counter]);
    }
    for (int counter = 0; counter < numberOfCustomers; counter++)
    {   
        pthread_join(orders[counter], &status);         
    }
           
    printf("-------------------------statistics-------------------------\n");
    printf("Total earnings: %d\n", earnings);
    printf("Successful orders: %d\n", successful_orders);
    printf("Failed orders: %d\n", failed_orders);
    printf("Average order time: %d minutes\n", avg_wait_time/numberOfCustomers);
    printf("Maximum order time: %d minutes\n", max_wait_time);
    printf("Average service time: %d minutes\n", avg_service_time/successful_orders);
    printf("Maximum service time: %d minutes\n", max_service_time);
    printf("Average cooling time: %d minutes\n", avg_cooling_time/successful_orders);
    printf("Maximum cooling time: %d minutes\n", max_cooling_time);
    printf("------------------------------------------------------------\n");

    return 1;

}