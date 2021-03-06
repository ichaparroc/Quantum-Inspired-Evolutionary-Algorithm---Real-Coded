/*ISRAEL CHAPARRO
QIEA-R*/

//#include <iostream.h>

#include <iostream>
#include <random>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <vector>

using namespace std;

///Mersenne Twister
typedef std::mt19937 MyRNG;  // the Mersenne Twister with a popular choice of parameters
uint32_t seed_val;           // populate somehow
MyRNG rng;                   // e.g. keep one global instance (per thread)
void initialize()
{
  rng.seed(seed_val);
}
//Mersenne Twister

pair<double,int> get_min(vector<double>&fit_t) //get min and position
{
	double min=fit_t[0];
	int idx=0;

	for(int i=1;i<(int)fit_t.size();i++)
		if(fit_t[i]<min)
		{
			min=fit_t[i];
			idx=i;
		}

	pair<double,int> ans;
	ans.first = min;
	ans.second = idx;

	return ans;
}

void Q_initialization(vector<vector<vector<double>>>&Q_t,double U_LIM,double L_LIM)
{
	//Same Domain
	for(int i=0;i<(int)Q_t.size();i++)
		for(int j=0;j<(int)Q_t[0].size();j++)
		{
			Q_t[i][j][0]=U_LIM-L_LIM; //sigma -- pulse width
			Q_t[i][j][1]=L_LIM+(U_LIM-L_LIM)/2.0; //mu -- square pulse
		}
}

void C_generation(vector<vector<double>>&C_t,vector<vector<vector<double>>>&Q_t)
{
	uniform_real_distribution<double> dist(0,1);
	for(int i=0;i<(int)Q_t.size();i++)
		for(int j=0;j<(int)Q_t[0].size();j++)
			C_t[i][j]=dist(rng)*Q_t[i][j][0]+(Q_t[i][j][1]-Q_t[i][j][0]/2.0);
}


/*
		//inicializa Qt con m individuos de n genes.
		//while t <= T do
			//generar Et observando los individuos Qt
			//if t = 1 then
			//	Ct = Et
			//else
			//recombinar(Et,Ct) -> Et
			evaluar Et
			seleccionar Ct<-k mejores individuos de Et U Ct
			end if
			actualizar Qt+1 con los m mejores individuos de Ct
			t = t + 1
		end while
*/

void evaluation(vector<double>&fit_t,vector<vector<double>>&C_t)
{
	double sum,a,b;
	for(int i=0;i<(int)C_t.size();i++)
	{
		sum=0.0;
		for(int j=0;j<(int)C_t[0].size();j++)
			sum+=pow(C_t[i][j],2);
		a=pow(sin(sqrt(sum)),2)-0.5;
		b=pow(1+0.001*sum,2);
		fit_t[i]=0.5+(a/b);
	}
}

int main(int argc,char **argv)
{

	int T_POB=1000,N_DIM=2,U_LIM=10,L_LIM=-10,N_GEN=1000;
	double P_CRO=0.5,P_UD=0.2;

	double best_fit;
	int best_pos;

	vector<vector<vector<double>>>Q_t(T_POB,vector<vector<double>>(N_DIM,vector<double>(2)));

	vector<vector<double>>C_t(T_POB,vector<double>(N_DIM));
	vector<vector<double>>E_t(T_POB,vector<double>(N_DIM));

	vector<double>fit_t(T_POB);

	uniform_int_distribution<int>dist_T_POB(0,T_POB-1); 	//To get index of classical individuals
	uniform_real_distribution<double>dist_01(0,1);	//To get prob of crossover

	//t=1
	int t=0;

	//Initialize Q(t) with m individuals from n genes
	Q_initialization(Q_t,U_LIM,L_LIM);

	//while t<=T
	while(t<N_GEN)
	{
		//generate E(t) observing the Q(t) individuals
		C_generation(E_t,Q_t);

		//if t=1 then
		if(t==0)
		{
			//C(t)=E(t)
			C_t=E_t;
			evaluation(fit_t,C_t);
			best_fit=get_min(fit_t).first;
			best_pos=get_min(fit_t).second;
		}
		//else
		else
		{
			//recombine {E(t),C(t)} -> E(t) -- Using Differential Evolution: crossover -- only crossover, no mutation because the quantum individuals have a random featuring for exploration

			int father,mother;	//parents
			vector<double> x_c1(N_DIM);//To save crossover1
            vector<double> x_c2(N_DIM);//To save crossover2

			for(int i=0;i<T_POB;i++)
			{
				double random=dist_01(rng);
				if(random<=P_CRO)
				{
					do
						father=dist_T_POB(rng);
					while(father==i||father==best_pos);

					do
						mother=dist_T_POB(rng);
					while(mother==i||mother==father||mother==best_pos);

					for(int j=0;j<N_DIM;j++)
					{
	                    double alpha=dist_01(rng);
	                    x_c1[j]=alpha*E_t[father][j]+(1-alpha)*C_t[mother][j];
	                    x_c2[j]=alpha*C_t[mother][j]+(1-alpha)*E_t[father][j];
					}

					E_t[father]=x_c1; //for all dimensions :)
	                E_t[mother]=x_c2; //for all dimensions :)
				}
			}

			//evaluate E(t)
			evaluation(fit_t,E_t); //Evaluation of E_t

			best_fit=get_min(fit_t).first;
			best_pos=get_min(fit_t).second;

			for(int dim=0;dim<2;dim++)
				cout<<E_t[best_pos][dim]<<" ";
			cout<<" : "<<best_fit<<endl;

	        //select C(t) according gap generational strategy -- aplicate elitism, implicit in recombination (crossover election of parents)
	        C_t=E_t;
		}

		//update Q(t+1) with the individuals of C(t)
        double random=dist_01(rng);

        if(random<P_UD && t!=0)
        {
            //Pulse width adjustment (sigma)-- Rule 1/5th
            double delta=dist_01(rng);
            int counter = 0;

            for(int i=0;i<T_POB;i++)
				if(fit_t[i]<best_fit)
					counter++;
            double phi=counter/T_POB;

            double mul_fact = 1.0;

            if(phi<0.2)
                mul_fact=delta;
            else if(phi>0.2)
                mul_fact=1.0/delta;

            for(int i=0;i<T_POB;i++)
                for( int j=0;j<N_DIM;j++)
                    Q_t[i][j][0]*=mul_fact;

        	//Moving Square pulse (mu)
            double lambda=dist_01(rng);

            for (int i=0;i<T_POB;i++)
                for (int j=0;j<N_DIM;j++)
                    Q_t[i][j][1]+=lambda*(C_t[best_pos][j]-Q_t[i][j][1]);
        }

		//t=t+1
		t++;
	}
}
