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
