#include <math.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define DM_E 		2.71828182845904523536
#define DM_LN2 		0.69314718055994530942
#define DM_SQRT2 	1.41421356237309504880
#define DM_SQRT3 	1.73205080756887729352
#define DM_PI		3.14159265358979323846
#define DM_PI_2		1.57079632679489661923

// Compute relative epsilon
double sigdigToReps(unsigned int sigdig) {
	double reps = 1;
	for (unsigned int i = 0; i < sigdig; i++) {
		reps = reps / 10;
	}
	return reps;
}

// Etox power series
double _ps_etox(double reps, double x) {
	double y = x;
	double s = 1 + y;
	unsigned int i = 2;
	while (fabs(y) > reps * fabs(s)) {
		y = y * x / i;
		s += y;
		i++;
	}
	return s;
}

// etox with Relative epsilon
double re_etox(double reps, double x) {
	if (! isfinite(x)) {
		if ( isnan(x) ) {
			return NAN;
		} else if ( x > 0 ) {
			// positive infinity
			return INFINITY;
		} else {
			// negative infinity
			return 0;
		}
	}
	double s = 1;
	double k;
	double dw;
	double d = modf(x,&dw);
	if (x > 0) {
		k = DM_E;
		if ( dw > DBL_MAX_EXP ) return INFINITY;
	} else if (x < 0) {
		k = 1 / DM_E;
		if ( dw < DBL_MIN_EXP ) return 0;
	} else {
		return 1;
	}
	int w = (int)fabs(dw);
	// compute the whole part of the exponent
	for ( int i = 0; i < w; i++ ) {
		if ( isinf(s) ) {
			return INFINITY;
		} else if ( s == 0 ) {
			return 1;
		}
		s = s * k;
	}
	s = s * _ps_etox(reps,d);
	return s;
}

// etox wrapper for sigdig
double etox(unsigned int sigdig, double x) {
	return re_etox(sigdigToReps(sigdig), x);
}

double re_etox_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r = re_etox(reps, va_arg(args, double));
	va_end(args);
	return r;
}

double _ps_lnx(double x, double reps) {
	double y = x - 1;
	double ox = y;
	double s = y;
	unsigned int i = 2;
	while (fabs(y) > reps * fabs(s)) {
		y = - y * ox * (i - 1) / i;
		s += y;
		i++;
	}
	return s;
}

double re_lnx(double reps, double x) {
	if ( x < 0 ) return NAN;
	if ( x == 0 ) return -INFINITY;
	if ( isinf(x) ) return INFINITY;
	double s = 0;
	int exp;
	double c = frexp(x, &exp);
	if (( c < 0.75 ) && ( exp > DBL_MIN_EXP )) {
		c = c * 2;
		exp--;
	}
	s = exp * DM_LN2;
	s += _ps_lnx(c,reps);
	return s;
}

double lnx(unsigned int sigdig, double x) {
	return re_lnx(sigdigToReps(sigdig),x);
}

double re_lnx_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r = re_lnx(reps, va_arg(args, double));
	va_end(args);
	return r;
}

double re_powxa(double reps, double x, double a) {
	if ( ( isnan(x) ) || ( isnan(a) ) ) return NAN;
	if ( a == 0 ) return 1;
	// Negative x handling
	if ( x < 0 ) {
		if ( isinf(a) ) {
			if ( x > -1 ) {
				if ( a > 0 ) return 0;
				else return INFINITY;
			} else if ( x == -1 ) {
				return NAN;
			} else {
				if ( a > 0 ) return NAN;
				else return 0;
			}
		}
		double dw;
		double d = modf(a,&dw);
		if ( d != 0 ) {
			return NAN;
		}
		double k;
		if ( a > 0 ) {
			k = x;
		} else {
			k = 1 / x;
		}
		double s = 1;
		int w = (int)fabs(dw);
		for (int i = 0; i < w; i++) {
			s = s * k;
		}
		return s;
	} else if ( x == 1 ) {
		return 1;
	}
	return re_etox(reps, a * re_lnx(reps, x));
}

double powxa(unsigned int sigdig, double x, double a) {
	return re_powxa(sigdigToReps(sigdig),x,a);
}

double re_powxa_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r;
	double d1 = va_arg(args, double);
	double d2 = va_arg(args, double);
	r = re_powxa(reps, d1, d2);
	va_end(args);
	return r;
}

double re_sqrtx(double reps, double x) {
	if ( x < 0 ) return NAN;
	double s = 1;
	int exp;
	int nexp;
	double c = frexp(x, &exp);
	nexp = exp / 2;
	s = ldexp(1.0,nexp);
	if (abs(exp % 2) == 1) {
		if (exp > 0) {
			s = s * DM_SQRT2;
		} else {
			s = s / DM_SQRT2;
		}
	}
	s = s * re_powxa(reps, c, 0.5);
	return s;
}

double sqrtx(unsigned int sigdig, double x) {
	return re_sqrtx(sigdigToReps(sigdig),x);
}

double re_sqrtx_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r = sqrtx(reps, va_arg(args, double));
	va_end(args);
	return r;
}

double re_argsinhx(double reps, double x) {
	return re_lnx(reps, x + re_sqrtx(reps, x * x + 1.0));
}

double argsinhx(unsigned int sigdig, double x) {
	return re_argsinhx(sigdigToReps(sigdig),x);
}

double re_argsinhx_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r = re_argsinhx(reps, va_arg(args, double));
	va_end(args);
	return r;
}

double _ps_arctgx(double x, double reps) {
	double y = x;
	double s = y;
	unsigned int i = 3;
	while (fabs(y) > reps * fabs(s)) {
		y = - y * x * x * (i - 2) / i;
		s += y;
		i += 2;
	}
	return s;
}

double re_arctgx(double reps, double x) {
	double s = 0;
	double ox = x;
	if ( x < 0 ) {
		ox = -x;
	}
	if ( ox > 1 ) {
		ox = 1/x;
		if ( ox > (2 - DM_SQRT3) ) {
			ox = ((DM_SQRT3 - 1) * ox + ox - 1) / (DM_SQRT3 + ox);
			s = DM_PI_2 - DM_PI/6 - _ps_arctgx(ox,reps);
		} else {
			s = DM_PI_2 - _ps_arctgx(ox,reps);
		}
	} else {
		if ( ox > (2 - DM_SQRT3) ) {
			ox = ((DM_SQRT3 - 1) * ox + ox - 1) / (DM_SQRT3 + ox);
			s = DM_PI/6 + _ps_arctgx(ox,reps);
		} else {
			s = _ps_arctgx(ox,reps);
		}
	}
	if ( x < 0 ) {
		s = -s;
	}
	return s;
}

double arctgx(unsigned int sigdig, double x) {
	return re_arctgx(sigdigToReps(sigdig),x);
}

double re_arctgx_va(double reps, ... ) {
	va_list args;
	va_start(args, reps);
	double r = re_arctgx(reps, va_arg(args, double));
	va_end(args);
	return r;
}

/*
	strToUint
	@desc: Converts string to unsigned int
	@return: If successfull 0
*/
int strToUint(char* str, unsigned int* a) {
	int i = 0;
	unsigned int result = 0;
	while ( str[i] != '\0' ) {
		if ( ( str[i] < '0' ) || ( str[i] > '9' ) ) return 1;
		if ( ((unsigned long long int)result * 10 + (unsigned char)str[i] - (unsigned char)'0') > UINT_MAX ) return 2;
		result = result * 10 + (unsigned char)str[i] - (unsigned char)'0';
		i++;
	}
	*a = result;
	return 0;
}

const char* P_HELPMSG =
	"Compugram v0.1\n"
	"Author: Jan Spisiak xspisi03@stud.fit.vutbr.cz\n"
	"Program for computing mathematical functions.\n\n"
	"  Usage:\n"
	"    ./proj1 [-h] (--powxa,--argsinh,--arctg) sigdig [(double)a]\n"
	"  Options:\n"
	"    -h\n"
	"        This help message.\n\n"
	"    --powxa\n"
	"    --argsinh\n"
	"    --arctg\n"
	"    sigdig\n"
	"        Significant digits.\n\n"
	"    a\n"
	"        Exponent of powxa.\n";

// Return values
enum P_RET {
	PR_OK = EXIT_SUCCESS,
	PR_ECLARGS,
	PR_EUNKNOWN,
	PR_EMALLOC
};

// Return Messages
const char* P_RET_MSGS[] =
{
	[PR_OK] = "Program exited successfully.\n",
	[PR_ECLARGS] = "Bad program arguments, please see -h.\n",
	[PR_EUNKNOWN] = "Unknown error. It shouldn't happen.\n",
	[PR_EMALLOC] = "Couldn't allocate enough memory.\n",
};

// Exit function
void p_exit( enum P_RET P_RETCODE ) {
	if ( P_RETCODE != PR_OK ) {
		printf("Exit %d: %s",P_RETCODE,P_RET_MSGS[P_RETCODE]);
	}
	exit( P_RETCODE );
}

// Writes help and exits
void p_help() {
	printf("%s",P_HELPMSG);
	p_exit(PR_OK);
}

// Verbosity levels
enum P_LOG_LEVEL {
	PLL_ERROR,
	PLL_WARNING,
	PLL_NOTICE,
	PLL_MSG
};

// The global variable we need
unsigned int PA_VERBOSITY = PLL_WARNING + 1;

// Prefix of logging entries
const char* P_LOG_LEVEL_MSGS[] =
{
	[PLL_ERROR] = 	"E: ",
	[PLL_WARNING] = "W: ",
	[PLL_NOTICE] = 	"N: ",
	[PLL_MSG] = 	"",
};

// Logging function
void p_log( enum P_LOG_LEVEL ll, const char* msg, ... ) {
	if ( ll < PA_VERBOSITY ) {
		va_list args;
		va_start( args, msg );
		fprintf( stderr, "%s", P_LOG_LEVEL_MSGS[ll] );
		vfprintf( stderr, msg, args );
		fprintf( stderr, "\n");
		va_end( args );
	}
}

struct p_arguments {
	double (*computeFunction)(double reps, ... );
	unsigned int sigdig;
	double* pd;
};

void p_resolveArguments(int argc, char** argv, struct p_arguments* pa) {
	if (( argc < 2 ) || (strcmp(argv[1],"-h") == 0)) {
		p_help();
	}

	if (argc < 3) {
		p_log(PLL_ERROR,"Not enough arguments.");
		p_exit(PR_ECLARGS);
	}

	if (strcmp(argv[1],"--powxa") == 0) {
		pa->computeFunction = re_powxa_va;
		if (argc < 4) {
			p_log(PLL_ERROR,"Not enough arguments. You need to specify the exponent. See -h");
			p_exit(PR_ECLARGS);
		} else {
			char* pch;
			*(pa->pd) = strtod(argv[3],&pch);
			if (*pch != '\0') {
				p_log(PLL_ERROR,"Wrong argmuent a. Should be double.");
				p_exit(PR_ECLARGS);
			}
		}
	} else if (strcmp(argv[1],"--etox") == 0) {
		pa->computeFunction = re_etox_va;
	} else if (strcmp(argv[1],"--ln") == 0) {
		pa->computeFunction = re_lnx_va;
	} else if (strcmp(argv[1],"--sqrt") == 0) {
		pa->computeFunction = re_sqrtx_va;
	} else if (strcmp(argv[1],"--argsinh") == 0) {
		pa->computeFunction = re_argsinhx_va;
	} else if (strcmp(argv[1],"--arctg") == 0) {
		pa->computeFunction = re_arctgx_va;
	} else {
		p_log(PLL_ERROR,"Wrong argument \"%s\".",argv[1]);
		p_exit(PR_ECLARGS);
	}

	if ((strToUint(argv[2],&pa->sigdig) != 0) || (pa->sigdig > DBL_DIG)) {
		p_log(PLL_ERROR,"Wrong sigdig, using default DBL_DIG = %d. See -h",DBL_DIG);
		pa->sigdig = DBL_DIG;
	}
}

double p_compute(struct p_arguments* pa, double reps, double x) {
	if (pa->pd != NULL) {
		return pa->computeFunction(reps, x, *(pa->pd) );
	} else {
		return pa->computeFunction(reps, x );
	}
}

int main(int argc, char** argv)
{
	struct p_arguments pa;
	double d;
	pa.pd = &d;
	p_resolveArguments(argc, argv, &pa);

	double x;
	int rscanf;
	double reps = sigdigToReps(pa.sigdig);
	while ((rscanf = scanf("%le",&x)) != EOF) {
		if (rscanf != 1) {
			scanf("%*s");
			continue;
		}
		printf("%.10e\n",p_compute(&pa, reps, x));
	}
    return EXIT_SUCCESS;
}
