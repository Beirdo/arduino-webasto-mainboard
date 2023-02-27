#ifndef __ntc_thermistor_h_
#define __ntc_thermistor_h_

class NTCThermistor {
  public:
    NTCThermistor(int t0, int t1, int r0, int r1, const int *table, int count, int offset);
    int32_t lookup(int rth);
    int32_t calculate(int rth);

  protected:
    const int *_table;
    int _count;
    int _offset;

    double _r0;
    double _t0;
    double _beta;
};

#endif
