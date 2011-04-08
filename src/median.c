/**
 * @file
 * @author Andrew Clegg
 *
 * Implementation of an efficient algorithm for finding the median of an unsorted list of numbers.
 */
#include <stdlib.h>

#include "median.h"
#include "data_handling.h"

/**
 * Partition a sublist of numbers (between first and last).
 *
 * Partioning a list involves selecting a pivot (the first number in the last), and partially
 * sorting the list until all values lesser than the pivot are positioned before the pivot,
 * and all values greater than the pivot are position after the pivot. The index of the pivot
 * in its final position is returned.
 * This is the same partition algorithm as used in Quicksort.
 *
 * @param values An array of NUMERIC_WORKING_TYPE floating point numbers.
 * @param first The index of the first item in the selected sublist.
 * @param last The index of the last item in the selected sublist.
 * @return The index of the pivot in its final position.
 */
static int partition(NUMERIC_WORKING_TYPE *values, int first, int last)
{
   if (first == last) return first;

   NUMERIC_WORKING_TYPE pivot_value = values[first];
   register int i = first;
   register int j = last+1;
   register NUMERIC_WORKING_TYPE swap_temp; //Used for swapping values

   do {

      do {i++;} while ((values[i]<pivot_value) && (i!=last));
      do {j--;} while ((values[j]>pivot_value) && (j!=first));

      if (i<j)
      {
         swap_temp = values[i];
         values[i] = values[j];
         values[j] = swap_temp;
      }
   } while (i<j);

   values[first] = values[j];
   values[j] = pivot_value;
   return j;
}

/**
 * Select the kth value from an ascending ordered sublist.
 *
 * This function is equivalent to performing a sort on the list, and then selecting the kth item.
 * However, this function only partially sorts the list until the desired item is known.
 *
 * @param values An array of NUMERIC_WORKING_TYPE floating point numbers.
 * @param k The index of the desired item; first <= k <= last.
 * @param first The index of the first item in the selected sublist.
 * @param last The index of the last item in the selected sublist.
 * @return The value of the kth item.
 */
static NUMERIC_WORKING_TYPE single_selection(NUMERIC_WORKING_TYPE *values, int k, int first, int last)
{
   int j;

   while(1)
   {
      // Partition the list
      j = partition(values, first, last);

      // Is this the position we are looking for?
      if (k==j) {
         // Found the right position - return the associated value
         return values[j];
      } else if (k<j) {
         // The partition was greater than the position we are looking for
         // Restrict the search to the positions lesser than the partition
         last = j-1;
      } else {
         // The partition was lesser than the position we are looking for
         // Restrict the search the positions greater than the partition
         first = j+1;
      }
   }
}

/**
 * Select the mean of the k1th and k2th value from an ascending ordered sublist.
 *
 * This function is equivalent to performing a sort on the list, and then selecting the k1th and k2th item.
 * However, this function only partially sorts the list until the desired item is known.
 * The following constraints apply to k1 and k2: first <= k1 < k2 <= last; k2 - k1 = 1.
 *
 * @param values An array of NUMERIC_WORKING_TYPE floating point numbers.
 * @param length The length of the array.
 * @param k1 The index of the first desired item.
 * @param k2 The index of the second desired item.
 * @return The mean of the values of k1 and k2.
 */
static NUMERIC_WORKING_TYPE mean_double_selection(NUMERIC_WORKING_TYPE *values, int length, int k1, int k2)
{
   int first = 0;
   int last = length-1;
   int j;

   while(1)
   {
      // Partition the data
      j = partition(values, first, last);


      if (k2<j) {
         // Partition is greater than both search positions,
         // so restrict the search to values left of the partition
         last = j-1;
      } else if (k1>j) {
         // Partition is lesser than both search positions,
         // so restrict the search to values right of the partition
         first = j+1;
      } else if (k1==j) {
         // Found the first value, so return the mean of this
         // and the second value
         return (values[j] + single_selection(values, j+1, j+1, length -1)) / 2.0;
      } else if (k2==j) {
         // Found the second value, so return the mean of this
         // and the first value
         return (single_selection(values, j-1, 0, j-1) + values[j]) / 2.0;
      } else {
         //Shouldn't get here!
         fprintf(stderr, "A logical error occurred in the mean_double_selection algorithm - this is most likely a bug in Caspian.\n");
         exit(-1);
      }
   }
}

/**
 * Compute the median of a list of values.
 *
 * @param values The list of values to compute the median of.
 * @param length The number of items in the list.
 * @return The median of the list.
 */
NUMERIC_WORKING_TYPE median(NUMERIC_WORKING_TYPE *values, int length)
{
   // Shortcuts for the trivial cases
   if (length==1) return values[0];
   if (length==2) return (values[0] + values[1]) / 2.0;

   if (length % 2 == 0) {
      return mean_double_selection(values, length, (length/2) - 1, (length/2));
   } else {
      return single_selection(values, (length - 1)/2, 0, length-1);
   }
}
