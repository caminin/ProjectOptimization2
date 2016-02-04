#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include <sys/timeb.h>
#include <string.h>

struct s_item{
unsigned int id;
int a;
int c;
char bestsol; /* '0' unselected, '1' selected, '?' undefined */
};
typedef struct s_item item;

struct s_node{
int var_id;  /* local constraint: x[var_ir] sign rhs */
char sign;   /* local constraint: x[var_ir] sign rhs */
int rhs;     /* local constraint: x[var_ir] sign rhs */
char status; /* 'n' new (=unsolved), 'f' fractional, 'i' integer 'u' unfeasible */
double obj;  /* Objective value of the corresponding node */
int var_frac;/* index of the fractional item (vaiable) is status == 'f' */ 
struct s_node *pred;
struct s_node *suc0;
struct s_node *suc1;
};
typedef struct s_node * TREE;

struct s_queue{
struct s_node * ptrnode;
struct s_queue * prev;
struct s_queue * next;
};
typedef struct s_queue * QUEUE;

char verbose = 'v'; /* 'v' for verbose */

/*********************************************************/
/*                     integerProfit                     */
/*********************************************************/
char integerProfit(int n, item * it)
{
int j = 0;
while(j < n)
   {
   if(it[j].c != floor(it[j].c))
      return '0';
   j++;
   }
return '1';
}

static int compare_item_2 (void const *a, void const *b)
{
   /* definir des pointeurs type's et initialise's
      avec les parametres */
   item const *it1 = (item const *)a;
   item const *it2 = (item const *)b;

   /* evaluer et retourner l'etat de l'evaluation (tri croissant) */
   return (it1->id > it2->id);
}

/*********************************************************/
/*                displayOptimalSolution                 */
/*********************************************************/
void displayOptimalSolution(int n, item * it)
{
int j;
int nb=0;
double cap=0,res=0;
qsort(it,n,sizeof(item),compare_item_2);
for(j = 0; j < n; j++)
   {
   if(it[j].bestsol == '1')
      {
        printf("\nItem %d which id is %d, size = %d and profit %d is selected.", j, it[j].id, it[j].a, it[j].c);
        cap+=it[j].a;
        res+=it[j].c;
        nb++;
      }
   }
   printf("\n Total de noeuds dans la sol %d",nb);
   printf("\n Taille de la sol %f",cap);
printf("\n Total res sol %f",res);   
}


/*********************************************************/
/*                    deleteNodeTree                     */
/*********************************************************/
/* Deletes node at address *tree as well as all the      */
/* successors of that node in the arborescence.          */
/* Also updates suc0 or suc1 in its parent node by       */
/* setting it to NULL. Finally, memory is freed and      */
/* *tree is set to NULL                                  */
/*********************************************************/
void deleteNodeTree(TREE *tree)
{
if(*tree == NULL) return ;
/* Deletion of the tree leaves first */
if ((*tree)->suc0 != NULL)
   deleteNodeTree(&(*tree)->suc0);
if ((*tree)->suc1 != NULL)
   deleteNodeTree(&(*tree)->suc1);
/* At this point, *tree is the address of a leaf */
/* So its parent must be updated before *tree is deleted */
if((*tree)->pred != NULL)
   {
   if((*tree)->rhs == 0)
      (*tree)->pred->suc0 = NULL;
   else if((*tree)->rhs == 1)
      (*tree)->pred->suc1 = NULL;
   else
      printf("\nError in deleteNodeTree : (*tree)->rhs = %d while it should be 0 or 1.\n", (*tree)->rhs);
   }
free(*tree);
*tree = NULL;
}



/*********************************************************/
/*                    deleteNodeQueue                    */
/*********************************************************/
/* Deletes node at address q in a queue starting at      */
/* address *queue. This address can be modified if       */
/* q == *queue (so queue is passed by address)           */
/*********************************************************/
void deleteNodeQueue(QUEUE *queue, QUEUE q)
{
if(q == NULL || *queue == NULL) return;
if((*queue)->next == NULL && q == *queue)
   {
   /* Deletion of a queue having a single element */
   free(q);
   *queue = NULL;
   return;
   }
if(q == *queue)
   {
   /* Deletion of the first element of the queue having at least 2 elements */
   *queue = q->next;
   q->next->prev = *queue;
   (*queue)->prev = NULL;
   free(q);
   return ;
   }
if(q->next == NULL)
   {
   /* Deletion of the last element of the queue having at least 2 elements */
   q->prev->next = NULL;
   free(q);
   return;
   }
/* Deletion of an element that is neither the first one, nor the last one of the queue. */
/* ie the queue has at least 3 elements */
q->prev->next = q->next;
q->next->prev = q->prev;
free(q);
}


/*********************************************************/
/*                      displayNode                      */
/*********************************************************/
void displayNode(TREE t)
{
printf("\nThis is displayNode, called for a node which address is %p.", t);
if(t != NULL)
   {
   printf("\n  Status: '%c'   obj = %lf", t->status, t->obj);
   printf("\n  Local constraint: x[%d] %c %d", t->var_id, t->sign, t->rhs);
   printf("\n  Fractional variable: x[%d] (applicable for status='f' only)", t->var_frac);
   printf("\n  predecessor at address %p", t->pred);
   printf("\n  suc0 at address %p and suc1 at address %p", t->suc0, t->suc1);
   }
}


/*********************************************************/
/*                      displayTree                      */
/*********************************************************/
void displayTree(TREE t)
{
//printf("\nThis is displayTree, called for a 'root' node which address is %p.", t);
if(t != NULL)
   {
   if(t->suc0 != NULL)
      displayTree(t->suc0);
   if(t->suc1 != NULL)
      displayTree(t->suc1);
   displayNode(t);
   }
}



/*********************************************************/
/*                     displayQueue                      */
/*********************************************************/
void displayQueue(QUEUE q)
{
printf("\nThis is displayQueue");
while(q != NULL)
   {
   displayNode(q->ptrnode);
   q = q->next;
   }
printf("\n**** end of displayQueue ****");
}

/*********************************************************/
/*                      sizeQueue                        */
/*********************************************************/
/* returns the number of open problems in the queue      */
/*********************************************************/
int sizeQueue(QUEUE q)
{
int size_queue = 0;
while(q != NULL)
   {
   q = q->next;
   size_queue++;
   }
return size_queue;
}


/*********************************************************/
/*                      addToQueue                       */
/*********************************************************/
void addToQueue(QUEUE *queue, TREE t)
{
QUEUE q, tmp, pred;
q = (QUEUE)malloc(sizeof(struct s_queue));
if(q == NULL)
   {
   printf("\nMemory allocation problem with the queue.\nEXITING.\n");exit(EXIT_FAILURE);
   }
q->ptrnode = t;
if(*queue == NULL)
   {
   /* the queue is empty, add the node as the head of the queue */
   q->prev = NULL;
   q->next = NULL;
   *queue = q;
   }
else if(q->ptrnode->obj > (*queue)->ptrnode->obj) /* CHANGED SIGN TO > */
   {
   /* q becomes the first element of the queue */
   /* pointers settings */
   (*queue)->prev = q;
   q->prev = NULL;
   q->next = *queue;
   /* swapping q and *queue */
   tmp = q;
   q = *queue;
   *queue = tmp;
   }
else
   {
   /* tmp browses the queue, searching for a node having an obj value */
   /* < q->ptrnode->obj if there exist one. */
   pred = NULL;
   tmp = (*queue);
   while(tmp != NULL && tmp->ptrnode->obj >= q->ptrnode->obj)
      {
      pred = tmp;
      tmp = tmp->next;
      }
   /* if tmp == NULL, q is the last element of the queue */
   /* otherwise, q must be inserted before tmp */
   if(tmp != NULL)
      {
      pred->next = q;
      q->prev = pred;
      tmp->prev = q;
      q->next = tmp;
      }
   else
      {
      /* q is the last element of the queue */
      /* pred is the current last element of the queue */
      pred->next = q;
      q->next = NULL;
      q->prev = pred;
      }
   }

}


/*********************************************************/
/*                         prune                         */
/*********************************************************/
/* This function deletes the nodes (in *q as well as in  */
/* tree) whose objective value is less than or equal to  */
/* *bestobj                                              */
/* intdata is 1 if all the costs are integer (allows to  */
/* prune at ceil(*bestobj)                               */
/*********************************************************/
void prune(QUEUE *q, double * bestobj, char intdata)
{
QUEUE tmp, qlast = *q;
if(verbose == 'v')
   printf("\n\n\nPRUNE with *bestobj = %lf\n\n\n",*bestobj);

if(verbose == 'v')
   {
   printf("\nBeginning of prune(), dispaying queue.");
   displayQueue(*q);
   }


if(*q == NULL) return;
while(qlast->next != NULL)
   qlast = qlast->next;
/* q last is the last open problem, i.e the open problem */
/* with the smallest objective value */


while(qlast != NULL && (intdata=='1'?floor(qlast->ptrnode->obj):qlast->ptrnode->obj) <= *bestobj)
   {
   //printf("\nNODE %p (obj = %lf) PRUNED", qlast->ptrnode, qlast->ptrnode->obj);
   tmp = qlast -> prev;   
   deleteNodeTree(&(qlast->ptrnode));
   deleteNodeQueue(q, qlast);
   qlast = tmp;
   }
if(verbose == 'v')
   {
   printf("\nEnd of prune(), dispaying queue.");
   displayQueue(*q);
   }
}


/*********************************************************/
/*                      displayData                      */
/*********************************************************/
void displayData(int n, int b, item *it)
{
    int j;
    printf("\nLoaded data:");
    printf("\nThere are n = %d items, and the knapsack capacity is b = %d.", n, b);
    for(j = 0; j < n; j++)
       printf("\nItem %d has id %u, its size is %d and its profit is %d", j, it[j].id, it[j].a, it[j].c);
}


/*********************************************************/
/*                     loadInstance                      */
/*********************************************************/
/* This function opens file *filename, reads *n, the     */
/* number of items, *b the knapsack capacity, and        */
/* allocates memory for *it, an array of *n item data    */
/* structures.                                           */
/*********************************************************/

int get_file_size(char *file_name)
{
   FILE *fp;
   int file_size;
   file_size = 0;
   if ((fp = fopen(file_name, "rb" )) == NULL) {
      fprintf(stderr, "Cannot open %s.\n", file_name);
      return(file_size);
   }
   char ligne[30];
   while(fgets(ligne, 30,fp) != NULL)
    {
	    file_size++;
    }
   fclose(fp);
   return(file_size);
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result =(char**) malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}


void loadInstance(char* filename, int *n, int *b, item **it)
{
    
    
    int size=get_file_size(filename);	
	FILE *fichier;
    fichier=fopen(filename,"r");
	
	if (fichier != NULL)
	{		
		char ** text=(char**)malloc(sizeof(char*)*size);
		char ligne[30];
		int num_ligne;
		for(num_ligne=0; fgets(ligne, 30,fichier) != NULL; ++num_ligne)
		{
			text[num_ligne]=(char*)malloc(sizeof(char)*(strlen(ligne)+1));
			strcpy(text[num_ligne],ligne);
		}
		
		char **ligne1=str_split(text[0],' ');
        
		int nb_noeuds=atoi(ligne1[0]);
        int capacity=atoi(ligne1[1]);
        
        printf("%d %d",nb_noeuds,capacity);
        
        *it=(item*)malloc(sizeof(item)*nb_noeuds);
		
		free(ligne1);
		
		int i;
		for(i=1;i<nb_noeuds+1;i++)
		{
			char **ligne1=str_split(text[i],' ');
            
            unsigned id=(unsigned)atoi(ligne1[0]);
			int size=atoi(ligne1[1]);
			int cost=atoi(ligne1[2]);
			
            (*it)[i-1].id=id;
            (*it)[i-1].a=size;
            (*it)[i-1].c=cost;
			free(ligne1);
		}
		
		for (i= 0; i < size; ++i){
		    free(text[i]);
		}
		free(text);
		fclose(fichier);
        
        *n=nb_noeuds;
        *b=capacity;
	}
	else
	{
	  // On affiche un message d'erreur si on veut
	  fprintf(stderr, "%s\n","Impossible d'ouvrir le fichier \n");
	}
}


/*********************************************************/
/*                     displaySol                        */
/*********************************************************/
void displaySol(int n, int b, item *it, char *x, double objx)
{
int j, load = 0;;
printf("\nDisplaying a solution with objx = %lg",objx);
for(j = 0; j < n; j++)
   {
   if(x[j] == '1')
      {
      printf("\n  Item with id = %u, size = %d and profit = %d, selected.", it[j].id, it[j].a, it[j].c);
      load += it[j].a;
      }
   else if(x[j] == '?')
      {
      printf("\n  Item with id = %u, size = %d and profit = %d, PARTIALLY selected.", it[j].id, it[j].a, it[j].c);
      load = b;
      }
   }
printf("\nKnapsack capacity %d, solution load = %d", b, load);

}

/*********************************************************/
/*                   solveRelaxation                     */
/*********************************************************/
/*                                                       */
/* The items in it[] are expected to be sorted by        */
/* decreasing utility BEFORE calling solveRelaxation     */
/* this function returns 'u' if problem is unfeasible    */
/*                       'i' if solution is integer      */
/*                       'f' if it is fractional         */
/* In the last case only, frac_item contains the index   */
/* of the item in the sorted list (not the id), that is  */
/* partially selected.                                   */
/* Input variables:                                      */
/* n is the number of items                              */
/* b is the knapsack capacity                            */
/* it is an array of items                               */
/* constraint is an array of n char, where constraint[j] */
/* is '1' if item j (in the sorted list) has to be       */
/* selected, '0' is not.                                 */
/* Output variables:                                     */
/* x is an array of n char, where x[j] is '1' if item j  */
/* is selected, '0' otherwise                            */
/* objx is a pointeur to a double containing the         */
/* objective value of solution x.                        */
/* frac_item is a pointeur to the item that is only      */
/* partially inserted in the knapsack, *frac_item = -1   */
/* if the solution is feasible (no fractional items)     */
/*********************************************************/
char solveRelaxation(int n, int b, item *it, char *constraint, char *x, double *objx, int *frac_item)
{
    
    int i;
    int capacity=b;
    *objx=0;
    for(i=0;i<n;i++)
    {
        x[i]=constraint[i];
        if(constraint[i]=='1')
        {
            capacity=capacity-it[i].a;
            *objx=*objx+it[i].c;
        }
        if(capacity<0)
        {
            return 'u';
        }
    }
    //8552, 2583
    for(i=0;i<n;i++)
    {
        if(constraint[i]=='F')
        {
            if(capacity==0)
            {
                *frac_item=-1;
                return 'i';
            }
            else if(capacity<it[i].a)
            {
                double to_add=((double)capacity/(double)(it[i].a)) *it[i].c;
                *objx=*objx+to_add;
                *frac_item=i;
                x[i]='?';
                
                if(verbose=='v')
                {
                    printf("J'affiche capacité actuelle du sac, cost du l'item et ce que j'ajoute (>0) '%d,%d,%d,%f\n",capacity,it[i].a,it[i].c,to_add);
                    printf("J'ai trop mis dans le sac\n");
                }
                int j;
                for(j=i+1;j<n;j++)
                {
                    if(x[j]=='?')
                    {
                        x[j]='0';
                    }
                }
                
                return 'f';
            }
            
            x[i]='1';
            capacity=capacity-it[i].a;
            *objx=*objx+it[i].c;
        }
    }
    printf("\nPas assez d'élément pour remplir le sac\n");
    return 'i';
}


/*********************************************************/
/*                  generateConstraint                   */
/*********************************************************/
/* returns an already dimensioned array constraints      */
/* by backtraking up to the root node                    */
/* p is a pointer to the current node                    */
/*********************************************************/
void generateConstraint(int n, TREE p, char *constraint)
{
int j;
/* Initializing the constraints to 'F' : all the items are free */
for(j = 0; j < n; j++)
   constraint[j] = 'F';

while(p->pred != NULL)
   {
   if(p->sign == '=')
      {
      if(p->rhs == 0)
         constraint[p->var_id] = '0';
      else if (p->rhs == 1)
         constraint[p->var_id] = '1';
      else
         printf("\nUnable to update array constraint because p->rhs is neither 0, nor 1.\n");
      }
   else
      printf("\nUnable to update array constraint because p->sign is not '='.\n");

   p = p->pred;
   }

/* Printing the constraints at the current node */
if(verbose == 'v')
   {
   printf("\n");
   for(j = 0; j < n; j++)
      printf("%c",constraint[j]);
   }
}


/*********************************************************/
/*                    createNode                         */
/*********************************************************/
void createNode(int n, int b, item *it, char intdata, char *x, char *constraint, TREE *newnode, TREE pred, int var_id, char sign, int rhs, double *bestobj, QUEUE *queue, unsigned int *nbnode)
{
int frac_item; /* index (not the id) in array it[] of the item j */
char status;   /* solveRelaxation status: 'i', 'u', or 'f'       */
double objx;   /* solution value associated with solution x      */
int j;

*newnode = (TREE)calloc(1, sizeof(struct s_node));
if(*newnode == NULL)
   {
   printf("\nMemory allocation problem, failing to create a tree newnode.\nExiting...\n");
   exit(EXIT_FAILURE);
   }
(*newnode)->pred = pred;
(*newnode)->var_id = var_id;
(*newnode)->sign = sign;
(*newnode)->rhs = rhs;
(*newnode)->var_frac = -1; /* fracional variable unidentified yet */
(*newnode)->status = 'n';

/* Solving the node, for the purpose of determining its objective value: */
/* (*newnode)->obj */

generateConstraint(n, *newnode, constraint);

status = solveRelaxation(n, b, it, constraint, x, &objx, &frac_item);
if(verbose == 'v')
   {
   switch(status)
      {
      case 'i': printf("\nsolveRelaxation solution is feasible (integer), with objx = %lf", objx);
      break;
      case 'u': printf("\nsolveRelaxation is unfeasible");
      break;
      case 'f': printf("\nsolveRelaxation solution is fractional with objx = %lf.\nItem %d having id %d, size %d and cost %d is used PARTIALLY in the solution, so branching is required.", objx, frac_item, it[frac_item].id, it[frac_item].a, it[frac_item].c);
      break;
      default: printf("\n\nUNEXPECTED CHAR VALUE RETURNED BY solveRelaxation: \"%c\"\n\n",status);
      }
   }
(*newnode)->status = status;
(*newnode)->obj = objx;

//displaySol(n, b, it, x, objx);
//char integerProfit(int n, item * it)


/* A new node is created only if its status is 'f' and its objective is stricly larger than *bestobj */
if(status == 'f' && *bestobj < (intdata == '1'?floor(objx):objx))
   {
   (*nbnode)++;
   (*newnode)->var_frac = frac_item;
   /* Updating the parent node too */
   if((*newnode)->pred != NULL)
      {
      if((*newnode)->rhs == 0)
         (*newnode)->pred->suc0 = (*newnode);
      else if((*newnode)->rhs == 1)
         (*newnode)->pred->suc1 = (*newnode);
      else
         {
         printf("\nError in createNode(): cannot update the parent node because the passed rhs = %d whereas it should be 0 or 1.\n", rhs);
         }
      }
   if(verbose == 'v')
      displayNode(*newnode);
   /* Insert the current node in the queue, at the correct location */
   addToQueue(queue, *newnode);
   //printf("\nSize of queue = %d elements", sizeQueue(*queue));
   }
else
   {
   if(verbose == 'v')
      printf("\nNo node creation (status is '%c')", (*newnode)->status);
   if(status == 'f')
      {
      if(verbose == 'v')
         printf("\nDespite its status 'f', this node is pruned because its objective value is %lf, whereas the objective value of the best feasible solution found so far is %lf.", objx, *bestobj);
      }
   else if(status == 'u')
      {
      /* Delete node, update its parent too. */
      if(pred == NULL)
         {
         /* The root node is unfeasible */
         printf("\nThis knapsack problem instance is unfeasible.");
         }
      }
   else if(status == 'i')
      {
      /* Compare objx to the best available integer solution,      */
      /* and update it if necessary. The best solution is directly */
      /* coded in the item structure, see char bestsol.            */
      /* Delete node, update its parent too.                       */
      if(objx > *bestobj)
         {
         *bestobj = objx;
         for(j = 0; j < n; j++)
            it[j].bestsol = x[j];
         }
      }
   
   /* Updating the parent node pred, if it exists */
   if(pred != NULL)
      {
      if(rhs == 0)
         pred->suc0 = NULL;
      else if(rhs == 1)
         pred->suc1 = NULL;
      else
         printf("\nError with rhs, that is %d while it should be either 0 or 1.\n", rhs);
      }
   /* Current node deletion */
   free(*newnode);
   }
}


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




/*********************************************************/
/*                        BB                             */
/*********************************************************/
void BB(int n, int b, item *it, double *bestobj)
{
char intdata; /* set to '1' if all items profits are integer */
char *constraint; /* constraint[j] = 'F' if item j (in the index number after sorting the index by decreasing utility) is not subject to a branching constraint (free). '0' if item j is not selected, '1' if it is selected */
char *x; /* solution returned by solveRelaxation, '0', '1', '?' */
/* such that x[j] = '?' ie the fractional item. */
double initial_best_obj; /* for storing *bestobj before solving an open problem, and see if its value has increased or not. If yes, we call prune() */
TREE p, root = NULL;   /* tree node */
QUEUE q, queue = NULL; /* queue of open problems (i.e. nodes for which */
/* the objective value is known, and that have a fractional item)   */
unsigned int nbTreeNodes = 0;


constraint = (char*)calloc(n, sizeof(char));
x = (char*)calloc(n, sizeof(char));
if(constraint == NULL || x == NULL)
   {
   printf("\nMemory allocation problem.\nExiting...\n");
   exit(EXIT_FAILURE);
   }

/* Setting intdata */
intdata = integerProfit(n, it);

/* Sorting the items by decreasing utility */
qsort(it,n,sizeof(item),compare_item);


/* Branch-and-Bound starts here */

/* creating root node */
createNode(n, b, it, intdata, x, constraint, &root, NULL, -1, 's', -1, bestobj, &queue, &nbTreeNodes);
//printf("\nThe queue of open problems has %d elements.", sizeQueue(queue));
//printf("\nThe best obj is %lf", *bestobj);

/* Here, we should decide what to do with the queue of open problems. */


while (queue != NULL)
   {
   /* Solve the first open problem in the queue. */
   initial_best_obj = *bestobj;
   q = queue;
   createNode(n, b, it, intdata, x, constraint, &p, q->ptrnode, q->ptrnode->var_frac, '=', 0, bestobj, &queue, &nbTreeNodes);
   createNode(n, b, it, intdata, x, constraint, &p, q->ptrnode, q->ptrnode->var_frac, '=', 1, bestobj, &queue, &nbTreeNodes);
   deleteNodeQueue(&queue, q);

   /* At this point, we should check that an integer solution has not been found, because in that case, it could be advantageous to delete (from the queue and in the tree) those open problems that have a .obj value less than or equal to *bestobj */
   if(initial_best_obj < *bestobj)
      prune(&queue, bestobj, intdata);

   /* DEBUG */
   if(verbose == 'v')
      {
      printf("\nIn BB, this is displayTree, called for a 'root' node which address is %p.", root);
      displayTree(root);
      printf("\nIn BB, the size of the queue is %d", sizeQueue(queue));
      displayQueue(queue);
      /* DEBUG END */
      }

   } /* end while(queue != NULL) */

printf("\nNumber of tree nodes: %u", nbTreeNodes);
free(constraint);
free(x);
}

void mywrite(item * it,int n,int b)
{
    FILE *f=fopen("./glpsol/Data.dat","w");
    
        fputs("data;\n",f);
        fputs("#volume maximum du sac-à-dos\n",f);
        fprintf(f,"param M := %d;\n",b);  
        fputs("#nombre de blocs\n",f);
        fprintf(f,"param n := %d;\n",n);  
        fputs("#taille des blocs\n",f);
        
        
        fprintf(f,"param t := %d %d\n",it[0].id,it[0].a);
        
        int i;
        for(i=1;i<(n-1);i++)
        {
          fprintf(f,"\t\t%d %d\n",it[i].id,it[i].a);  
        }
        fprintf(f,"\t\t%d %d;\n",it[i].id,it[i].a);  
        
        fputs("#valeur des blocs\n",f);
        fprintf(f,"param v := %d %d\n",it[0].id,it[0].c);
        
        for(i=1;i<(n-1);i++)
        {
          fprintf(f,"\t\t%d %d\n",it[i].id,it[i].c);  
        }
        fprintf(f,"\t\t%d %d;\n",it[i].id,it[i].c);
        fputs("end;\n",f);
        
            
    fclose(f);
}


/*********************************************************/
/*                       main                            */
/*********************************************************/
int main(int argc, char* argv[])
{
int n, b;
item *it = NULL;
double bestobj = 0; /* objective value */
struct timeb t0, t1;    /* Timers */
double cpu_time;        /* Time */


ftime(&t0);
if(argc != 2)
   {
   printf("\nCall the program with an argument, that is the instance file name.\n");
   exit(EXIT_FAILURE);
   }

loadInstance(argv[1], &n, &b, &it);
//if(verbose == 'v')
//   displayData(n, b, it);

BB(n, b, it, &bestobj);

printf("\nOptimal objective value is %lf",bestobj);
//if(verbose == 'v')
   displayOptimalSolution(n, it);

 //Below, for checking that the items have been reordered by decreasing utility. */
//displayData(n, b, it);

ftime(&t1);
cpu_time = (double)(t1.time - t0.time) + (double)(t1.millitm-t0.millitm)/1000.0;
printf("\nCPU time : %f seconds.", cpu_time);

mywrite(it,n,b);

free(it);
printf("\n");
return EXIT_SUCCESS;
}
