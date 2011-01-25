#include "kd_tree.h"
#include "median.h"

static int partition(float *values, int first, int last)
{
   if (first == last) return first;

   float V = values[first];
   register int i = first;
   register int j = last+1;
   register float swapTemp; //Used for swapping values

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

static float evenSelection(float *values, int k, int first, int last)
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

static float oddSelection(float *values, int lenA, int k1, int k2)
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
         return (values[j] + evenSelection(values, j+1, j+1, lenA -1)) / 2.0;
      }
      else if (k2==j)
      {
         return (evenSelection(values, j-1, 0, j-1) + values[j]) / 2.0;
      }
      else
      {
         //Shouldn't get here!
         exit(1);
      }
   }
}

float median(float *values, int lenA)
{
   if (lenA==1) return values[0];
   if (lenA==2) return (values[0] + values[1]) / 2.0;
   if (lenA%2==0) return oddSelection(values,lenA,(lenA/2)-1,(lenA/2));
   return evenSelection(values, (lenA-1)/2, 0, lenA-1);
}
