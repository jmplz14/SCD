#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;






//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
const unsigned int NUMERO_FUMADORES = 3;
Semaphore estanquero = 1;
Semaphore fumadores[NUMERO_FUMADORES] = {0,0,0};
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
int producir(){
		unsigned int num_objeto = aleatorio<0,2>();
		chrono::milliseconds duracion_producir(aleatorio<20,200>());
		this_thread::sleep_for(duracion_producir);
		
		
		return num_objeto;
	
}
void funcion_hebra_estanquero(  )
{
	unsigned int num_objeto;
	while(true){
		sem_wait(estanquero);
		num_objeto = producir();
		cout << "producido ingrediente:" << num_objeto << endl;
		sem_signal(fumadores[num_objeto]);
		
		
	}
	
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
	while(true){
		sem_wait(fumadores[num_fumador]);
		cout << "retirado ingrediente: " << num_fumador << endl;
		sem_signal(estanquero);
		fumar(num_fumador);
	}
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   // ......
	thread hebra_estanquero(funcion_hebra_estanquero);
	thread hebra_fumadores[NUMERO_FUMADORES];
	for(int i = 0; i < NUMERO_FUMADORES; i++)
		hebra_fumadores[i]= thread(funcion_hebra_fumador, i);
		
	hebra_estanquero.join();
	for(int i = 0; i < NUMERO_FUMADORES; i++)
		hebra_fumadores[i].join();
}
