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
 *
 * @param values An array of NUMERIC_WORKING_TYPE floating point numbers.
 * @param first The index of the first item in the selected sublist.
 * @param last The index of the last item in the selected sublist.
 * @return The index of the pivot in its final position.
 */
static int partition(NUMERIC_WORKING_TYPE *values, int first, int last)
{
   if (first == last) return first;

   NUMERIC_WORKING_TYPE V = values[first];
   register int i = first;
   register int j = last+1;
   register NUMERIC_WORKING_TYPE swapTemp; //Used for swapping values

   do {

      do {i++;} while ((values[i]<V) && (i!=last));
      do {j--;} while ((values[j]>V) && (j!=first));

      if (i<j)
      {
         swapTemp = values[i];
         values[i] = values[j];
         values[j] = swapTemp;
      }
   } while (i<j);

   values[first] = values[j];
   values[j] = V;
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
static NUMERIC_WORKING_TYPE singleSelection(NUMERIC_WORKING_TYPE *values, int k, int first, int last)
{
   int j;

   while(1)
   {
      j = partition(values, first, last);
      if (k==j) return values[j];
      else if (k<j) last = j-1;
      else first = j+1;
   }
}

/**
 * Select the mean of the k1th and k2th value from an ascending ordered sublist.
 *
 * This function is equivalent to performing a sort on the list, and then selecting the k1th and k2th item.
 * However, this function only partially sorts the list until the desired item is known.
 *
 * @param values An array of NUMERIC_WORKING_TYPE floating point numbers.
 * @param lenA The length of the array.
 * @param k1 The index of the first desired item; first <= k1 <= last.
 * @param k2 The index of the second desired item; first <= k2 <= last.
 * @return The mean of the values of k1 and k2.
 */
static NUMERIC_WORKING_TYPE meanDoubleSelection(NUMERIC_WORKING_TYPE *values, int lenA, int k1, int k2)
{
   int first = 0;
   int last = lenA-1;
   int j;
   while(1)
   {
      j = partition(values, first, last);
      if (k2<j) last = j-1;
      else if (k1>j) first = j+1;
      else if (k1==j)
      {
         return (values[j] + singleSelection(values, j+1, j+1, lenA -1)) / 2.0;
      }
      else if (k2==j)
      {
         return (singleSelection(values, j-1, 0, j-1) + values[j]) / 2.0;
      }
      else
      {
         //Shouldn't get here!
         exit(1);
      }
   }
}

/**
 * Compute the median of a list of values.
 *
 * @param values The list of values to compute the median of.
 * @param lenA The number of items in the list.
 * @return The median of the list.
 */
NUMERIC_WORKING_TYPE median(NUMERIC_WORKING_TYPE *values, int lenA)
{
   if (lenA==1) return values[0];
   if (lenA==2) return (values[0] + values[1]) / 2.0;
   if (lenA%2==0) return meanDoubleSelection(values,lenA,(lenA/2)-1,(lenA/2));
   return singleSelection(values, (lenA-1)/2, 0, lenA-1);
}
