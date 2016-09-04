#define _GNU_SOURCE 	/* Utilizado por sched.h */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>	
#include <sched.h>

#include "utility.h"
#include "minheap.h"
#include "maxheap.h"

#define TIME_TOL	0.1 /* Tolerancia de tempo */


void nextProcess(int id)
{
	switch (sched) {
		case 1:	/* First-Come First-Served */
			pthread_mutex_lock(&gmutex);
			pnext = pnext + 1;
			threadResume(pnext);
			pthread_mutex_unlock(&gmutex);
			break;
		case 2: /* Shortest Remaining Time Next */
			pthread_mutex_lock(&hmutex);
			maxHeapRemove(running, procs, id);
			pthread_mutex_unlock(&hmutex);
			break;
		default:
			printf("Erro na seleção do próximo processo.\n");
			exit(EXIT_FAILURE);
	}
	pthread_mutex_lock(&gmutex);
	tnumb--;
	pthread_mutex_unlock(&gmutex);
}

void *timeOperation(void *tid)
{
	int id;
	double elapsed, inactive;
	clock_t start, end;

	id = *((int *) tid);

	/* Se os processadores estao ocupados, esperar */
	if (tnumb < pnumb)
		threadResume(id);
	threadStatus(id);
	
	if (sched == 1) {
		pthread_mutex_lock(&gmutex);
		tnumb++; 
		pthread_mutex_unlock(&gmutex);
	}



	/* Simulacao de execucao */
	start = clock();
	while (TRUE) {
		inactive = threadStatusTime(id);
		if (inactive > 0) printf("%f\n", inactive);	
		end = clock();
		elapsed = ((double)end - (double)start) / CLOCKS_PER_SEC;
		elapsed -= inactive;
		start = end;
		procs[id].rt -= elapsed;
		if (procs[id].rt <= 0.0) { 
			procs[id].tf = ((double)end - (double)gstart) / CLOCKS_PER_SEC;
			break; 
		}
	}		

	if (dflag == 1) {
		printf("Processo da linha [%d]", procs[id].tl);  
		printf(" finalizado, escrito na linha [%d].\n", procs[id].tl);
	}
	pline++;

	nextProcess(id);
	return 0;
}

void firstCome(int n)
{
	int i = 0;
	clock_t end, elapsed;

	pnext = pnumb - 1; /* pnumb processos poderao rodar no inicio */

	/* Execucao das threads (ordenadas por ordem de chegada) */
	while (i < n) {
		end = clock();
		elapsed = ((double)end - (double)gstart) / CLOCKS_PER_SEC;

		/* Chegamos ao instante de chegada de uma thread */
		if (elapsed >= procs[i].at - TIME_TOL && elapsed <= procs[i].at + TIME_TOL) {
			if (dflag == 1) 
				printf("Processo da linha [%d] chegou ao sistema.\n", procs[i].tl);

			threadCreate(i);
			i++;
		}
	}
}

void shortestRemaining(int n)
{
	int i = 0;
	int topid, botid;
	clock_t end, elapsed;
	
	elapsed = 0;

	pthread_mutex_lock(&hmutex);
	ready = minHeapInit(n);
	running = maxHeapInit(pnumb);
	pthread_mutex_unlock(&hmutex);

	/* Execucao das threads (ordenadas por ordem de chegada) */
	while (pline < n) {
		end = clock();
		elapsed = ((double)end - (double)gstart) / CLOCKS_PER_SEC;

		pthread_mutex_lock(&hmutex);
		botid = minHeapTop(ready);
		topid = maxHeapTop(running);
		pthread_mutex_unlock(&hmutex);

		/* Chegamos ao instante de chegada de uma thread */
		if (elapsed >= procs[i].at - TIME_TOL && elapsed <= procs[i].at + TIME_TOL) {
			if (dflag == 1) 
				printf("Processo da linha [%d] chegou ao sistema.\n", procs[i].tl);		
			
			pthread_mutex_lock(&hmutex);
			minHeapInsert(ready, procs, i);
			pthread_mutex_unlock(&hmutex);
			
			threadCreate(i);
			i++;
		}

		/* Preempcao quando um dos processos em espera e menor que um executando */
		if (topid != -1 && botid != -1 && procs[topid].rt > procs[botid].rt) {
			printf("TROCA DO PROCESSO %d POR %d\n", topid, botid);
			printf("%f\n", (double)elapsed);
			printf("%f\n", procs[topid].rt);
			threadPause(topid);

			pthread_mutex_lock(&hmutex);
			minHeapPop(ready, procs);
			maxHeapInsert(running, procs, botid);
			maxHeapPop(running, procs);
			minHeapInsert(ready, procs, topid);
			pthread_mutex_unlock(&hmutex);

			threadResume(botid);
		}

		/* Processador disponivel e lista de espera nao vazia */
		else if (tnumb < pnumb && botid != -1) {
			printf("PROCESSADOR LIVRE, RODANDO PROCESSO %d\n", botid);
			printf("%f\n", (double)elapsed);
			printf("%f\n", procs[botid].rt);
			
			pthread_mutex_lock(&gmutex);
			tnumb++; 
			pthread_mutex_unlock(&gmutex);

			pthread_mutex_lock(&hmutex);
			minHeapPop(ready, procs);
			maxHeapInsert(running, procs, botid);
			pthread_mutex_unlock(&hmutex);
			
			threadResume(botid);
		}
	}
}