#ifndef __ntc_thermistor_h_
#define __ntc_thermistor_h_

#define UNUSED_READING (int32_t)(0x80000000)

class NTCThermistor {
  public:
    NTCThermistor(int t0, int t1, int r0, int r1, const long int *table, int count, int offset);
    int32_t lookup(int rth);
#ifdef FLOATING_POINT_MATH
    int32_t calculate(int rth);
#endif

  protected:
    const long int *_table;
    int _count;
    int _offset;

#ifdef FLOATING_POINT_MATH
    double _r0;
    double _t0;
    double _beta;
#endif
};


extern NTCThermistor thermistor;

#endif
