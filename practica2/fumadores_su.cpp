#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.hpp"
const int NUM_FUMADORES = 3;

using namespace std;
using namespace HM;

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
class Estanco : public HoareMonitor{
  private:
    int num_ingredientes, mostrador;
    CondVar *fumadores, estanquero;
  public:
    Estanco(int ingredientes);
    ~Estanco();
    void obtenerIngrediente(int i);
    void ponerIngrediente(int i);
    void esperarRecogidaIngredinete();

};

Estanco::Estanco(int ingredientes){
  num_ingredientes = ingredientes;
  fumadores = new CondVar[ingredientes];
  for(int i = 0 ; i < ingredientes ; i++)
    fumadores[i] = newCondVar();
  //fumador = newCondVar
  estanquero = newCondVar();
  mostrador = -1;
}

Estanco::~Estanco(){
  delete [] fumadores;
}

void Estanco::obtenerIngrediente(int i){
  assert (i < num_ingredientes || i >=0);
  while(mostrador != i)
    fumadores[i].wait();

    //fumador.wait();

  assert(mostrador == i);
  mostrador = -1;
  estanquero.signal();
}

void Estanco::ponerIngrediente(int i){
  assert (i < num_ingredientes || i >= 0);
  assert( mostrador == -1);
  mostrador = i;

}

void Estanco::esperarRecogidaIngredinete(){
    if(fumadores[mostrador].empty())
      estanquero.wait();

    if(mostrador != -1 )
      fumadores[mostrador].signal();

}

void fumar( int num_fumador ){

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
int producir(){
		unsigned int num_objeto = aleatorio<0,NUM_FUMADORES - 1>();
		chrono::milliseconds duracion_producir(aleatorio<20,200>());
		this_thread::sleep_for(duracion_producir);


		return num_objeto;

}

void funcion_hebra_estanquero( MRef<Estanco> monitor ){
	while(true){
    int ingrediente = producir();
    monitor->ponerIngrediente(ingrediente);
    cout << "producido ingrediente:" << ingrediente << endl;
    monitor->esperarRecogidaIngredinete();
	}

}

void funcion_hebra_fumador( MRef<Estanco> monitor, int i ){
	while(true){
    monitor->obtenerIngrediente(i);
    fumar(i);

	}

}

int main(){
  MRef<Estanco> monitor = Create<Estanco>(NUM_FUMADORES);
  thread hebras_fumadores[NUM_FUMADORES];

  for(int i = 0; i < NUM_FUMADORES; i++)
    hebras_fumadores[i] = thread ( funcion_hebra_fumador, monitor,i);

  thread hebra_estanquero(funcion_hebra_estanquero, monitor);

  for(int i = 0; i < NUM_FUMADORES; i++)
    hebras_fumadores[i].join();

  hebra_estanquero.join();
}
