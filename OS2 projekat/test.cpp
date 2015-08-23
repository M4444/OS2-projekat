#include "fs.h"
#include "file.h"
#include "part.h"
#include <iostream>

using namespace std;

int main()
{
	Partition *partition1, *partition2;
	char p1, p2;

	partition1 = new Partition("t1.ini");
	cout << "Kreirana particija" << endl;
	p1 = FS::mount(partition1);
	cout << "Montirana particija" << endl;
	FS::format(p1);
	cout << "Formatirana particija" << endl;


	{
		char filepath[] = "1:\\fajl1.dat";
		filepath[0] = p1;
		File *f = FS::open(filepath, 'w');
		cout << "Kreiran fajl 'fajl1.dat'" << endl;

		FILE *file = fopen("ulaz.dat", "rb");
		if(file == 0){
			cout << "GRESKA: Nije nadjen ulazni fajl 'ulaz.dat' u os domacinu!" << endl;
			system("PAUSE");
			return 0;//exit program
		}
		char *ulazBuffer = new char[32 * 1024 * 1024];//32MB
		int ulazSize = fread(ulazBuffer, 1, 32 * 1024 * 1024, file);
		f->write(ulazSize, ulazBuffer);
		cout << "Prepisan sadrzaj 'ulaz.dat' u 'fajl1.dat'" << endl;

		f->seek(0);
		char *toWr = new char[ulazSize + 1];
		f->read(0x7FFFFFFF, toWr);//namerno veci broj od velicine fajla
		toWr[ulazSize] = 0;//zatvaramo string pre ispisa
		cout << "Sadrzaj fajla 'fajl1.dat'" << endl;
		cout << toWr << endl;

		delete[] toWr;
		delete f;
		cout << "zatvoren fajl 'fajl1.dat'" << endl;
	}
}