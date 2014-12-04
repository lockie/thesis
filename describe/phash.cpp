
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "datasetIO.h"

#include "pHash.h"

using namespace std;


static char* tmpdir = NULL;
static char path[PATH_MAX];

static int descr_callback(const IplImage* img, int id, int frame,
		int x, int y, int _class, void* dataset)
{
	int r;
	char* errMsg;
	ulong64 hash;
	strncpy(path, tmpdir, PATH_MAX);
	strncat(path, "/tmp.png", PATH_MAX-strlen(tmpdir));

	if(cvSaveImage(path, img) == 0)
	{
		cerr << "Error writing image to '" << path << "'" << endl;
		return -1;
	}

	// TODO : try other hashes pHash provides:
	// Marr-Hildreth
	// uint8_t* ph_mh_imagehash(const char *filename, int &N, float alpha=2.0f, float lvl = 1.0f);
	// Radial Variance
	// int ph_image_digest(const char *file, double sigma, double gamma, Digest &digest,int N=180);
	// (DCT) Discrete Cosine Transform
	//int ph_dct_imagehash(const char* file,ulong64 &hash);
	if((r = ph_dct_imagehash(path, hash)) < 0)
	{
		cerr << "Error: pHash calculation failed" << endl;
		return r;
	}

	if((r = dataset_update_sample_descriptor(&dataset, id,
		(float*)&hash, sizeof(hash) / sizeof(float), &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		return r;
	}

	return 0;
}

extern "C" int describe_pHash(const char* dataset_path)
{
	int r;
	void* dataset = NULL;
	char* errMsg;
	if((r = dataset_open(&dataset, dataset_path, 0, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}

	if(!tmpdir)
	{
		struct stat st;
		if(stat("/dev/shm", &st) == 0 && S_ISDIR(st.st_mode))
			tmpdir = (char*)"/dev/shm";
		else
			if(stat("/tmp", &st) == 0 && S_ISDIR(st.st_mode))
				tmpdir = (char*)"/tmp";
		if(!tmpdir)
			tmpdir = (char*)".";
	}

	if((r = dataset_read_samples(&dataset, "1=1",
			descr_callback, dataset, &errMsg)) != 0)
	{
		cerr << "Error: " << errMsg << endl;
		dataset_close(&dataset);
		return r;
	}
	dataset_close(&dataset);
	return 0;
}

