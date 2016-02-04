qsort(it,n,sizeof(item),compare_item);

static int compare_item (void const *a, void const *b)
{
   /* definir des pointeurs type's et initialise's
      avec les parametres */
   item const *it1 = (item const *)a;
   item const *it2 = (item const *)b;

   /* evaluer et retourner l'etat de l'evaluation (tri croissant) */
   if(it1->c == it2->c)
   {
       return it1->a >= it2->a;
   }

   return (it1->c <= it2->c);
}


