// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con un único productor y un único consumidor.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


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

constexpr int
   num_items  = 40,
  num_productores = 4,
  num_consumidores = 4;     // número de items a producir/consumir

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items], // contadores de verificación: consumidos
   cont_producidos[num_consumidores] = {0};
//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra)
{

   int dato = num_hebra * (num_items/num_productores) + cont_producidos[num_hebra];
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << dato << endl << flush ;
   mtx.unlock();
   cont_prod[dato] ++ ;
   cont_producidos[num_hebra]++;

   return dato;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons1SC : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   num_leidas,
   num_producidas ;          //  indice de celda de la próxima inserción

 CondVar         // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons1SC(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SC::ProdCons1SC()
{
  num_leidas = 0 ;
  num_producidas = 0 ;
   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SC::leer(  )
{

  if (num_leidas == num_producidas)
    ocupadas.wait();

  assert( num_leidas < num_producidas  );

  const int valor = buffer[num_leidas % num_celdas_total] ;

  num_leidas++;

  libres.signal();
  /*mtx.lock();
  cout << "             leido: " << valor << endl << flush ;
  mtx.unlock();*/
  return valor;

}
// -----------------------------------------------------------------------------

void ProdCons1SC::escribir( int valor )
{

  if ((num_producidas - num_leidas) == num_celdas_total)
    libres.wait();

  assert( (num_producidas - num_leidas) < num_celdas_total  );
  buffer[num_producidas % num_celdas_total] = valor ;
  num_producidas++;


  ocupadas.signal();
  /*mtx.lock();
  cout << "escrito: " << valor << endl << flush ;
  mtx.unlock();*/
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdCons1SC> monitor, int hebra )
{
   for( unsigned i = 0 ; i < num_items/num_productores ; i++ )
   {

      int valor = producir_dato( hebra ) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons1SC> monitor )
{
   for( unsigned i = 0 ; i < num_items/num_consumidores ; i++ )
   {

      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (1 prod/cons, Monitor SC, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   //ProdCons1SC monitor ;
   MRef<ProdCons1SC> monitor = Create<ProdCons1SC>();

   thread hebra_productora[num_productores];
   thread hebra_consumidora[num_consumidores];

  for(int i = 0 ; i < num_productores; i++)
    hebra_productora[i] = thread ( funcion_hebra_productora, monitor,i);

  for(int i = 0; i < num_consumidores; i++)
    hebra_consumidora[i] = thread (funcion_hebra_consumidora, monitor);

    for(int i = 0 ; i < num_productores; i++)
      hebra_productora[i].join();

    for(int i = 0; i < num_consumidores; i++)
      hebra_consumidora[i].join();


   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
