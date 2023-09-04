/*
 * Swarthmore College, CS 31
 * Copyright (c) 2022 Swarthmore College Computer Science Department,
 * Swarthmore PA
 */

#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "colors.h"

/****************** Definitions **********************/
/* Three possible modes in which the GOL simulation can run */
#define OUTPUT_NONE   (0)   // with no animation
#define OUTPUT_ASCII  (1)   // with ascii animation
#define OUTPUT_VISI   (2)   // with ParaVis animation

/* Used to slow down animation run modes: usleep(SLEEP_USECS);
 * Change this value to make the animation run faster or slower
 */
//#define SLEEP_USECS  (1000000)
#define SLEEP_USECS    (100000)

/* A global variable to keep track of the number of live cells in the
 * world (this is the ONLY global variable you may use in your program)
 */
static int total_live = 0;

struct gol_data {

    // NOTE: DO NOT CHANGE the names of these 4 fields (but USE them)
    int rows;  // the row dimension
    int cols;  // the column dimension
    int iters; // number of iterations to run the gol simulation
    int output_mode; // set to:  OUTPUT_NONE, OUTPUT_ASCII, or OUTPUT_VISI

    int *array;
    

    /* fields used by ParaVis library (when run in OUTPUT_VISI mode). */
    visi_handle handle;
    color3 *image_buff;
};


/****************** Function Prototypes **********************/

/* the main gol game playing loop (prototype must match this) */
void play_gol(struct gol_data *data);

/* init gol data from the input file and run mode cmdline args */
int init_game_data_from_args(struct gol_data *data, char **argv);

// A mostly implemented function, but a bit more for you to add.
/* print board to the terminal (for OUTPUT_ASCII mode) */
void print_board(struct gol_data *data, int round);

// Checks if the (k,l) neighbors are live or not.
// returns the number of live neighbors
int check_neighbors(struct gol_data *data, int k, int l);

// Sets (k,l) to live or dead based on number of live neighbors (live_neighbors)
void set_alive(struct gol_data *data, int *array, int live_neighbors, int k, int l);

// updates the color for visi animation
// pasted from weeklylab code - modified to work here
void update_colors(struct gol_data *data);

/************ Definitions for using ParVisi library ***********/
/* initialization for the ParaVisi library (DO NOT MODIFY) */
int setup_animation(struct gol_data* data);
/* register animation with ParaVisi library (DO NOT MODIFY) */
int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data);
/* name for visi (you may change the string value if you'd like) */
static char visi_name[] = "GOL!";


int main(int argc, char **argv) {

    int ret;
    struct gol_data data;
    double secs;
    struct timeval start_time, stop_time;

    /* check number of command line arguments */
    if (argc < 3) {
        printf("usage: %s <infile.txt> <output_mode>[0|1|2]\n", argv[0]);
        printf("(0: no visualization, 1: ASCII, 2: ParaVisi)\n");
        exit(1);
    }

    /* Initialize game state (all fields in data) from information
     * read from input file */
    ret = init_game_data_from_args(&data, argv);
    if (ret != 0) {
        printf("Initialization error: file %s, mode %s\n", argv[1], argv[2]);
        exit(1);
    }

    /* initialize ParaVisi animation (if applicable) */
    if (data.output_mode == OUTPUT_VISI) {
        setup_animation(&data);
    }

    /* ASCII output: clear screen & print the initial board */
    if (data.output_mode == OUTPUT_ASCII) {
        if (system("clear")) { perror("clear"); exit(1); }
        print_board(&data, 0);
    }

    /* Invoke play_gol in different ways based on the run mode */
    if (data.output_mode == OUTPUT_NONE) {  // run with no animation
        ret = gettimeofday(&start_time, NULL);
        play_gol(&data);
        ret = gettimeofday(&stop_time, NULL);
    }
    else if (data.output_mode == OUTPUT_ASCII) { // run with ascii animation
        ret = gettimeofday(&start_time, NULL);
        play_gol(&data);
        ret = gettimeofday(&stop_time, NULL);

        // clear the previous print_board output from the terminal
        if (system("clear")) { perror("clear"); exit(1); }

        print_board(&data, data.iters);
    }
    else {  // OUTPUT_VISI: run with ParaVisi animation
            // tell ParaVisi that it should run play_gol
        connect_animation(play_gol, &data);
        // start ParaVisi animation
        run_animation(data.handle, data.iters);
    }

    if (data.output_mode != OUTPUT_VISI) {
        
        double seconds = stop_time.tv_sec - start_time.tv_sec;
        double micros = (stop_time.tv_usec - start_time.tv_usec)/1000000.0;
        secs = seconds + micros;

        /* Print the total runtime, in seconds. */
        fprintf(stdout, "Total time: %0.3f seconds\n", secs);
        fprintf(stdout, "Number of live cells after %d rounds: %d\n\n",
                data.iters, total_live);
    }

    free(data.array);

    return 0;
}

//Checks the amount of live neighbors that the coordinate (k,l) has
//returns the number of live neighbors
int check_neighbors(struct gol_data *data, int k, int l) {

    int num_neighbors, neighbor_row, neighbor_col, i, j;
    num_neighbors = 0;
    for (i = -1; i < 2; i++) {
        neighbor_row = (k + i + data->rows) % (data->rows);
        for (j = -1; j < 2; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            neighbor_col = (l + j + data->cols) % (data->cols);
            if (data->array[neighbor_row * data->cols + neighbor_col] == 1) {
                num_neighbors += 1;
            }
        }
    }

    return num_neighbors;
}

//sets the coordinate (i,j) to live if it meets the game requirements
//returns nothing: modifies the array as is. 
void set_alive(struct gol_data *data, int *array, int live_neighbors, int i, int j) {
    
    if (data->array[i*(data->cols)+j] == 1) {
        if (live_neighbors < 2 ) {
            total_live--;
        }
        else if (live_neighbors > 3) {
            total_live--;
        }
        else {
            array[i*(data->cols)+j] = 1;
        }
    }
    else {
        if (live_neighbors == 3) {
            array[i*(data->cols)+j] = 1;
            total_live++;
        }
    }
}

//updates the colors based on whether the coordinate is live or not
//copied and edited from visi_example in week notes
void update_colors(struct gol_data *data) {

    int i, j, r, c, index, buff_i;
    color3 *buff;

    buff = data->image_buff;  // just for readability
    r = data->rows;
    c = data->cols;

    for (i = 0; i < r; i++) {
        for (j = 0; j < c; j++) {
            index = i*c + j;
            buff_i = (r - (i+1))*c + j;

            // update animation buffer
            if (data->array[index] == 1) {
                buff[buff_i] = c3_pink;
            } else if (data->array[index] == 0) {
                buff[buff_i] = c3_black;
            } 
        }
    }
}

int init_game_data_from_args(struct gol_data *data, char **argv) {

    FILE *infile;
    int ret;

    infile = fopen(argv[1], "r");
    if (infile == NULL){
        printf("ERROR: failed to open file: %s\n", argv[1]);
        exit(1);
    }
    ret = fscanf(infile,"%d", &data->rows);
    if (ret ==0){
        printf("Error: failed to read rows");
        exit(1);
    }
    ret = fscanf(infile,"%d", &data->cols);
    if (ret == 0){
        printf("Error: failed to read cols");
        exit(1);
    }


    //creating dynamic array
    //then initializing elements to 0
    data->array = malloc(sizeof(int)*data->rows*data->cols);
    if (data->array == NULL) {
        printf("ERROR: array initialization malloc error");
        exit(1);
    }
    int i, j;
    for (i=0; i < data->rows; i++){
        for (j=0; j < data->cols; j++){
            data->array[i*data->cols+j] = 0;
        }
    }

    //reading the number of iterations to perform
    ret = fscanf(infile,"%d", &data->iters);
    if (ret ==0){
        printf("Error");
        exit(1);
    }
    //checking and setting output mode
    if (atoi(argv[2]) == 0){
        data->output_mode = OUTPUT_NONE;
    }
    else if (atoi(argv[2]) == 1){
        data->output_mode = OUTPUT_ASCII;
    }
    else if (atoi(argv[2]) == 2){
        data->output_mode = OUTPUT_VISI;
    }
    else{
        printf("Error: invalid print mode choice");
        exit(1);
    }

    //reading number of live cells
    ret = fscanf(infile, "%d", &total_live);
    if (ret == 0){
        printf("Error: failed to read number of live cells");
        exit(1);
    }
    
    //reading the live cells
    int num1, num2;
    for (i = 0; i < total_live; i++) {
        ret = fscanf(infile, "%d%d", &num1, &num2);
        if (ret == 0){
        printf("Error: failed to read live cell #%d", (i+1));
        exit(1);
        }
        data->array[num1*data->cols+num2] = 1;
    }

    //closing the file
    ret = fclose(infile);
    if (ret != 0) {
        printf("Error: failed to close file: %s\n", argv[1]);
        exit(1);
    }
    return 0;
}

/* the gol application main loop function:
 *  runs rounds of GOL,
 *    * updates program state for next round (world and total_live)
 *    * performs any animation step based on the output/run mode
 *
 *   data: pointer to a struct gol_data  initialized with
 *         all GOL game playing state
 */
void play_gol(struct gol_data *data) {


    //  at the end of each round of GOL, determine if there is an
    //  animation step to take based on the output_mode,
    //   if ascii animation:
    //     (a) call system("clear") to clear previous world state from terminal
    //     (b) call print_board function to print current world state
    //     (c) call usleep(SLEEP_USECS) to slow down the animation
    //   if ParaVis animation:
    //     (a) call your function to update the color3 buffer
    //     (b) call draw_ready(data->handle)
    //     (c) call usleep(SLEEP_USECS) to slow down the animation

    int live_neighbors, i, j, k;
    int *new_array;
    for (k = 1; k < data->iters; k++) {
        new_array = malloc(sizeof(int)*data->rows*data->cols);
        for (i = 0; i < data->rows; i++) {
            for (j = 0; j < data->cols; j++) {
                new_array[i*data->cols+j] = 0;
                live_neighbors = check_neighbors(data, i, j);
                set_alive(data, new_array, live_neighbors, i, j);
            }
        }
        free(data->array);
        data->array = new_array;
        if (data->output_mode == OUTPUT_ASCII) {
            system("clear");
            print_board(data, k);
            usleep(SLEEP_USECS);
        }
        else if (data->output_mode == OUTPUT_VISI) {
            update_colors(data);
            draw_ready(data->handle);
            usleep(SLEEP_USECS);
        }
    }   

}

void print_board(struct gol_data *data, int round) {

    int i, j;

    /* Print the round number. */
    fprintf(stderr, "Round: %d\n", round);

    for (i = 0; i < data->rows; ++i) {
        for (j = 0; j < data->cols; ++j) {
            if (data->array[i*data->cols+j] == 1) {
                fprintf(stderr, " @");
            }
            else {
                fprintf(stderr, " .");
            }
        }
        fprintf(stderr, "\n");
    }

    /* Print the total number of live cells. */
    fprintf(stderr, "Live cells: %d\n\n", total_live);
}


/**********************************************************/
/* initialize ParaVisi animation */
int setup_animation(struct gol_data* data) {
    /* connect handle to the animation */
    int num_threads = 1;
    data->handle = init_pthread_animation(num_threads, data->rows,
            data->cols, visi_name);
    if (data->handle == NULL) {
        printf("ERROR init_pthread_animation\n");
        exit(1);
    }
    // get the animation buffer
    data->image_buff = get_animation_buffer(data->handle);
    if(data->image_buff == NULL) {
        printf("ERROR get_animation_buffer returned NULL\n");
        exit(1);
    }
    return 0;
}

/* sequential wrapper functions around ParaVis library functions */
void (*mainloop)(struct gol_data *data);

void* seq_do_something(void * args){
    mainloop((struct gol_data *)args);
    return 0;
}

int connect_animation(void (*applfunc)(struct gol_data *data),
        struct gol_data* data)
{
    pthread_t pid;

    mainloop = applfunc;
    if( pthread_create(&pid, NULL, seq_do_something, (void *)data) ) {
        printf("pthread_created failed\n");
        return 1;
    }
    return 0;
}
