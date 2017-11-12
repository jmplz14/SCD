#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;

const int NUM_CLIENTES = 10;

class Barberia : public HoareMonitor{
  private:
    CondVar barbero, silla, clientes;

  public:
    Barberia();
    void cortarPelo(int i);
    void siguienteCliente();

    void finCliente();
};
Barberia::Barberia(){
  barbero = newCondVar();
  clientes = newCondVar();
  silla = newCondVar();
}

void Barberia::cortarPelo(int i){
  cout << "Entra el cliente " << i << endl;
  if(clientes.empty()){
    if(barbero.empty()){
      cout << "Cliente " << i << " espera que el barbero termine" << endl;
      clientes.wait();
    }else{
      cout << "Clinete " << i << " despierta al barbero" << endl;
      barbero.signal();
    }

  }else{
    cout << "Cliente " << i << " se pone a la cola" << endl;
    clientes.wait();
  }

  cout << "Cliente " << i << " se sienta el silla del barbero." << endl;
  silla.wait();


}

void Barberia::siguienteCliente(){
  if(clientes.empty()){
    cout << "                           Barbero se duerme(No hay clientes)" << endl;
    barbero.wait();
  }else{
    cout << "                           Barbero avisa al siguiente Clientes" << endl;
    clientes.signal();
  }


}

void Barberia::finCliente(){
  cout << "                           Barbero termino de pelar al cliente" << endl;
  silla.signal();
}

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void esperaFueraBarberia(){
  chrono::milliseconds duracion_espera( aleatorio<200,2000>() );
  this_thread::sleep_for(duracion_espera);
}

void cortarPeloCliente(){
  chrono::milliseconds duracion_pelado( aleatorio<200,2000>() );
  this_thread::sleep_for(duracion_pelado);
}

void funcion_hebra_barbero(MRef<Barberia> monitor){
  while(1){
    monitor->siguienteCliente();
    cortarPeloCliente();
    monitor->finCliente();
  }
}

void funcion_hebra_cliente(MRef<Barberia> monitor, int i){
  while(1){
    monitor->cortarPelo(i);
    cout << "Cliente " << i << " espera fuera " << endl;
    esperaFueraBarberia();
  }
}

int main(int argc, char const *argv[]) {
  MRef<Barberia> monitor = Create<Barberia>();

  thread hebras_clientes[NUM_CLIENTES];
  thread hebra_barbero(funcion_hebra_barbero, monitor);

  for(int i = 0; i < NUM_CLIENTES; i++)
    hebras_clientes[i] = thread ( funcion_hebra_cliente, monitor,i);


  for(int i = 0; i < NUM_CLIENTES; i++)
    hebras_clientes[i].join();

  hebra_barbero.join();






  return 0;
}
