#include <iostream> /* cout, cin, endl; FILE */
#include "fs.h"
#include "file.h"
#include "part.h"

using namespace std;

char *inputBuffer;
int inputSize;

Partition *partition;
char p;

int main(){
	cout << "Pocetak main funkcije" << endl;
	// ucitavanje ulaznog fajla u bafer, da bi mogao da se cita
	FILE *f = fopen("ulaz.dat", "rb");
	if(f == 0){
		cout << "***GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu***\a" << endl;
		system("PAUSE");
		return 0; //exit program
	}
	inputBuffer = new char[32 * 1024 * 1024]; //32MB
	inputSize = fread(inputBuffer, 1, 32 * 1024 * 1024, f);
	fclose(f);

	//1. blok niti 1
	partition = new Partition("p1.ini");
	cout << "Kreirana particija" << endl;
	p = FS::mount(partition);
	if(p >= 'A' && p <= 'Z') cout << "Montirana particija" << endl;
	else if(p == '0') cout << "***GRESKA: Nije nadjeno mesto za montiranje particije***\a" << endl;
	else cout << "***NEPOZNATA GRESKA: Montiranje particije nije uspelo***\a" << endl;
	FS::format(p);
	/*wait(mutex); cout << "Nit1: Formatirana particija" << endl; signal(mutex);
	{
		char filepath[] = "1:\\fajl1.dat";
		filepath[0] = p1;
		File *f = FS::open(filepath, 'w');
		wait(mutex); cout << "Nit1: Kreiran fajl 'fajl1.dat'" << endl; signal(mutex);
		f->write(ulazSize, ulazBuffer);
		wait(mutex); cout << "Nit1: Prepisan sadrzaj 'ulaz.dat' u 'fajl1.dat'" << endl; signal(mutex);
		f->seek(0);
		char *toWr = new char[ulazSize + 1];
		f->read(0x7FFFFFFF, toWr);//namerno veci broj od velicine fajla
		toWr[ulazSize] = 0;//zatvaramo string pre ispisa
		wait(mutex);
		cout << "Nit1: Sadrzaj fajla 'fajl1.dat'" << endl;
		cout << toWr << endl;
		signal(mutex);
		delete[] toWr;
		delete f;
		wait(mutex); cout << "Nit1: zatvoren fajl 'fajl1.dat'" << endl; signal(mutex);
	}
	wait(mutex); cout << "Nit1: signal 2" << endl; signal(mutex);
	signal(sem12);
	wait(mutex); cout << "Nit1: wait 3" << endl; signal(mutex);
	wait(sem31);//ceka nit 3
	*/
	cout << "Kraj main funkcije" << endl;
	return 0;
}