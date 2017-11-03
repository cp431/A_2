/**
  a2.c
  MPI implementation of the parallel merging algorithm.
  Contributors:
  -> Frank Khalil	  160226600
  -> Sarah Johnston	150139570
  -> Brad Katz		  130750210
  -> Gareth Sharpe	090361370
*/

// Includes
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// CONSTANTS
#define FIRST 0
#define RAND_STEP_SIZE 10
#define ARRAY_A_SIZE 1
#define SUB_ARRAY_A 2
#define ARRAY_B_SIZE 3
#define SUB_ARRAY_B 4

typedef struct {
  int *subarray_a_lengths;
  int *subarray_b_lengths;
  int *subarray_a_indices;
  int *subarray_b_indices;
} array_info;



// Static functions

/**
  Runs a binary search on a given array and returns the index of the last occurrence of num_to_find if it exists in the array.
  
  @param array Pointer to the array to search
  @param num_to_find The integer to search for in the array
  @param array_size The length of the array to search
  
  @return index_found The index of the last occurrence of num_to_find, or -1 if num_to_find does not exist in the array
*/
static int binary_search(const int array[], const int num_to_find, const int array_size)
{
    int first_index = 0;
    int last_index = array_size - 1;
    int midpoint = floor((first_index + last_index) / 2);
  
    // Assume error case
    int index_found = -1;
  
    while (first_index <= last_index)
    {
        if (array[midpoint] < num_to_find)
        {
            first_index = midpoint + 1;
        }
        else if (array[midpoint] == num_to_find)
        {
            index_found = midpoint;
            first_index = midpoint + 1;
        }
        else
        {
            last_index = midpoint - 1;
        }
      
        midpoint = floor((first_index + last_index) / 2);
    }
  
    return midpoint;
}

/**
  Generates two array_size arrays of ints.
  
  @param arr_a Pointer to the first array
  @param arr_b Pointer to the second array
  @param The length of the arrays as given on the command line
*/
static void gen_arrays(int arr_a[], int arr_b[], const int array_size)
{  
    // Seed RNG
    srand(time(NULL));
  
    // Generate the first element of each array
    arr_a[0] = rand() % RAND_STEP_SIZE;
    arr_b[0] = rand() % RAND_STEP_SIZE;
  
    for (int i = 1; i < array_size; ++i)
    {
        arr_a[i] = (rand() % ((i * RAND_STEP_SIZE) - arr_a[i - 1])) + arr_a[i - 1];
        arr_b[i] = (rand() % ((i * RAND_STEP_SIZE) - arr_b[i - 1])) + arr_b[i - 1];
    }
}

/**
  Prints a given array out to the console.
  Ex: [1, 2, 3, 4] is printed as 1, 2, 3, 4 followed by a newline.
  
  @param array The array to be printed
  @param array_size The length of the array
*/
static void print_array(const int array[], const int array_size)
{
    for (int i = 0; i < array_size - 1; ++i)
        printf("%d, ", array[i]);
  
    printf("%d\n", array[array_size - 1]);
}

static void partition_data(const int array_size, int* arr_a, int* arr_b, int num_processors, array_info *data) 
{
    int *arr_a_size, *arr_b_size, *arr_a_indices, *arr_b_indices;
    arr_a_size = (int*)calloc(num_processors, sizeof(int));
    arr_b_size = (int*)calloc(num_processors, sizeof(int));
    arr_a_indices = (int*)calloc(num_processors, sizeof(int));
    arr_b_indices = (int*)calloc(num_processors, sizeof(int));
    int remainder = array_size % num_processors;
    
    // determines the sizes required for each subarray of array a
    // calculated by divididing array length by # of processors
    // case 1: if no remainder of division, add value to arr_a_size array
    // case 2: if remainder, add value + 1 to arr_a_size and decrement remainder
    // result: arr_a_size contains all sub-array displacements of array a
    for (int i = num_processors - 1; i > -1; --i) 
    {
        arr_a_size[i] = array_size / num_processors;
        if (remainder > 0) {
          arr_a_size[i] += 1;
          --remainder;
        } 
    }
  
    // determines the indicies (i_start) for each processor required for array b
    // calculated by adding the array size of processor i and the starting index of processor i and
    // storing this value as the next index of the next processor
    // result: arr_a_indicies full of starting indicies of each sub array of array a
    int start = 0;
    int index;
    arr_a_indices[0] = start;
    for (int i = 1; i < num_processors; i++) 
    {
      index = arr_a_size[i-1] + arr_a_indices[i-1];
      arr_a_indices[i] = index;
    }
    
    // searches for the index of last occuring element in each subarray of b that is equal
    // to or less than the last element of each subarray of a
    // result: arr_b_indicies full of starting index of each sub array of array b and arr_b_size
    // full of the lengths of each sub arry of array b
    int index_of_key;
    int key;
    int size;
    for (int i = 0; i < num_processors; i++) 
    {
        index_of_key = arr_a_size[i] + arr_a_indices[i] - 1;
        key = arr_a[index_of_key];
        index = binary_search(arr_b, key, array_size);
        if (index > 0) {
            size = index + 1 - start;
            arr_b_indices[i] = start;
            arr_b_size[i] = size;
            start = index + 1;
        } else if (index == -1) {
            int size = 0;
            arr_b_indices[i] = start;
            arr_b_size[i] = size;
        }
        index = index + arr_a_size[i+1];
    }
    
    // malloc memory for array_data struct
    size_t size_of_array = (sizeof *data->subarray_a_lengths) * num_processors;
   
    // malloc memory for each attribute of array_data struct
    data->subarray_a_lengths = malloc(size_of_array);
    data->subarray_b_lengths = malloc(size_of_array);
    data->subarray_a_indices = malloc(size_of_array);
    data->subarray_b_indices = malloc(size_of_array);
    
    // initialize array_data struct with appropriate data
    data->subarray_a_lengths = arr_a_size;
    data->subarray_b_lengths = arr_b_size;
    data->subarray_a_indices = arr_a_indices;
    data->subarray_b_indices = arr_b_indices;
}

static void merge_arrays(int *sub_arr_a, int *sub_arr_b, int *sub_arr_c, int size_a, int size_b) 
{
    int array_index = 0, i = 0, j = 0;
      
    while (i < size_a && j < size_b)
    {
        if (sub_arr_a[i] < sub_arr_b[j])
            sub_arr_c[array_index++] = sub_arr_a[i++];
        else
            sub_arr_c[array_index++] = sub_arr_b[j++];
    }
    
    if (i == size_a)
        while (j < size_b)
            sub_arr_c[array_index++] = sub_arr_b[j++];   
    if (j == size_b)
      while (i < size_a)
            sub_arr_c[array_index++] = sub_arr_a[i++];
}

int is_sorted(int *array, int size)
{
  int i = 0; 
  for (; i < size - 1; i++)
  {
    if (array[i] > array[i + 1])
        return 0;
  }
  return 1;
}

int main(int argc, char *argv[])
{
  MPI_Status status;
  MPI_Request request;
  
  int *disp;
  int process_rank;
  int num_processors;
 
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processors);
  
  if (argc < 2)
  {
      if (process_rank == FIRST)
        printf("ERROR: Missing array length exponent. Usage: ./a2 [array length exponent]\n");
      
      MPI_Finalize();
      return 0;
  }
  
  int array_size = pow(2, atoi(argv[1]));
  int *arr_a = calloc(array_size, sizeof(int));
  int *arr_b = calloc(array_size, sizeof(int));
  
  gen_arrays(arr_a, arr_b, array_size);
  
  array_info *array_data = (array_info *)malloc(sizeof(array_info));
  partition_data(array_size, arr_a, arr_b, num_processors, array_data);
  
  int sub_arr_a_recv_count = array_data->subarray_a_lengths[process_rank];
  int sub_arr_b_recv_count = array_data->subarray_b_lengths[process_rank];
  
  int *sub_arr_a = (int *) malloc(sizeof(int) * sub_arr_a_recv_count);
  int *sub_arr_b = (int *) malloc(sizeof(int) * sub_arr_b_recv_count);
  
  // Scatter arr_a across all procs
  MPI_Scatterv(arr_a, array_data->subarray_a_lengths, array_data->subarray_a_indices, MPI_INT, sub_arr_a, sub_arr_a_recv_count, MPI_INT, FIRST, MPI_COMM_WORLD);
  
  // Scatter arr_b across all procs
  MPI_Scatterv(arr_b, array_data->subarray_b_lengths, array_data->subarray_b_indices, MPI_INT, sub_arr_b, sub_arr_b_recv_count, MPI_INT, FIRST, MPI_COMM_WORLD);
  
  // Merge sub_arr_a and sub_arr_b into sub_arr_c
  int sub_arr_c_length = sub_arr_a_recv_count + sub_arr_b_recv_count;
  int *sub_arr_c = (int *)malloc(sizeof(int) * sub_arr_c_length);
  merge_arrays(sub_arr_a, sub_arr_b, sub_arr_c, sub_arr_a_recv_count, sub_arr_b_recv_count);
  
  // Gather all sub_arr_c instances back to the root process
  int *arr_c = (int *)malloc(sizeof(int) * (array_size * 2));
  
  int *sub_arr_c_recv_counts = (int *)malloc(sizeof(int) * num_processors);
  int *sub_arr_c_indices = (int *)malloc(sizeof(int) * num_processors);
  
  for (int i = 0; i < num_processors; ++i)
  {
      sub_arr_c_recv_counts[i] = array_data->subarray_a_lengths[i] + array_data->subarray_b_lengths[i];
      sub_arr_c_indices[i] = array_data->subarray_a_indices[i] + array_data->subarray_b_indices[i];
  }
  
  MPI_Gatherv(sub_arr_c, sub_arr_c_length, MPI_INT, arr_c, sub_arr_c_recv_counts, sub_arr_c_indices, MPI_INT, FIRST, MPI_COMM_WORLD);
  
  if (process_rank == FIRST)
  {
    printf("Array A: ");
    print_array(arr_a, array_size);
    
    printf("Array B: ");
    print_array(arr_b, array_size);
    
    printf("Array C: ");
    print_array(arr_c, array_size * 2);
    
    printf("\nIs sorted: %d", is_sorted(arr_c, array_size));
  }
  
  free(sub_arr_c_recv_counts);
  free(sub_arr_c_indices);
  free(array_data->subarray_a_lengths);
  free(array_data->subarray_a_indices);
  free(array_data->subarray_b_indices);
  free(array_data->subarray_b_lengths);
  free(array_data);
  free(sub_arr_a);
  free(sub_arr_b);
  free(sub_arr_c);
  free(arr_a);
  free(arr_b);
  free(arr_c);
  
  MPI_Finalize();
  return 0;
}