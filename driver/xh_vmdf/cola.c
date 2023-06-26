#include <wdm.h>

#define _COLA_
#include "cola.h"
#undef _COLA_

PCOLA ColaCrear()
{
	PCOLA cola=(PCOLA)ExAllocatePoolWithTag(NonPagedPool, 
                                sizeof(COLA), 
                                (ULONG)'lcHV'
                                );
	if(cola!=NULL) {
		cola->fin=NULL;
		cola->principio=NULL;
	}
	return cola;
}

BOOLEAN ColaEstaVacia(PCOLA cola)
{
	if(cola->principio==NULL)
		return TRUE;
	else
		return FALSE;
}

VOID ColaBorrar(PCOLA cola)
{
	if(cola->principio!=NULL) {
		while(!ColaBorrarNodo1(cola));
	}
	ExFreePoolWithTag((PVOID)cola,(ULONG)'lcHV');
}

BOOLEAN ColaBorrarNodo1(PCOLA cola)
{
	PNODO primero=cola->principio;
	if(primero!=NULL) {
		cola->principio=primero->link;
		if(primero->link==NULL) cola->fin=cola->principio;
		if(primero->Datos!=NULL) ExFreePool(primero->Datos);
		ExFreePoolWithTag((PVOID)primero,(ULONG)'lcHV');
		if(cola->principio==NULL) {
			return TRUE; // Cola vacia
		}
	} else {
		return TRUE;
	}

	return FALSE; // Cola no vacia
}

BOOLEAN ColaPush(PCOLA cola,PVOID dato)
{
	PNODO nodo=(PNODO)ExAllocatePoolWithTag(NonPagedPool, 
                                sizeof(NODO), 
                                (ULONG)'lcHV'
                                );
	if(nodo!=NULL) {
		nodo->Datos=dato;
		nodo->link=NULL;
		if(cola->principio==NULL) {
			cola->principio=nodo;
		} else {
			cola->fin->link=nodo;
		}
		cola->fin=nodo;
	} else {
		return FALSE;
	}

	return TRUE;
}

/*
VOID ColaBorrarDesde(PCOLA cola,PNODO comienzo)
{
	PNODO siguiente;
	PNODO nodo;
	if(comienzo!=NULL) {
		nodo=comienzo->link;
		cola->fin=comienzo;
		comienzo->link=NULL;
	} else {
		nodo=cola->principio;
		cola->principio=cola->fin=NULL;
	}
	while(nodo!=NULL)
	{
		siguiente=nodo->link;
		if(nodo->Datos!=NULL) ExFreePool(nodo->Datos);
		ExFreePool((PVOID)nodo);
		nodo=siguiente;
	}
}*/

BOOLEAN ColaBorrarNodo(PCOLA cola,PNODO nodo, PNODO anterior)
{
	if(nodo==cola->principio || anterior==NULL)
	{
		return ColaBorrarNodo1(cola);
	} else {
		anterior->link=nodo->link;
		if(nodo==cola->fin) {
			cola->fin=anterior;
		}
		if(nodo->Datos!=NULL) ExFreePool(nodo->Datos);

		ExFreePoolWithTag(nodo,(ULONG)'lcHV');
	}

	return FALSE; // Cola no vacia
}