/* Copyright (C) 2003 Massachusetts Institute of Technology
%
%  This program is free software; you can redistribute it and/or modify
%  it under the terms of the GNU General Public License as published by
%  the Free Software Foundation; either version 2, or (at your option)
%  any later version.
%
%  This program is distributed in the hope that it will be useful,
%  but WITHOUT ANY WARRANTY; without even the implied warranty of
%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%  GNU General Public License for more details.
%
%  You should have received a copy of the GNU General Public License
%  along with this program; if not, write to the Free Software Foundation,
%  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef DACTYL_H
#define DACTYL_H

#include <complex>

#include "vec.h"

const double c = 0.5;
const double pi = 3.141592653589793238462643383276L;

class polarizability;
class polarization;
class grace;

class mat {
 public:
  double *eps, a;
  double *inveps[10];
  double *Cmain[10];
  double *Cother[10];
  volume v;
  polarizability *pb;
  const char *outdir;

  ~mat();
  mat(const volume &v, double eps(const vec &));
  mat(const mat *);
  mat(const mat &);
  void make_average_eps();
  void use_pml_left(double dx);
  void use_pml_right(double dx);
  void use_pml_radial(double dx);

  void output_slices(const char *name = "");
  void output_slices(const volume &what, const char *name = "");
  void set_output_directory(const char *name);
  void mix_with(const mat *, double);

  void add_polarizability(double sigma(const vec &), double omega, double gamma,
                          double delta_epsilon = 1.0, double energy_saturation = 0.0);
 private:
  void output_sigma_slice(const volume &what, const char *name);
  double pml_fmin;
};

class src;
class bandsdata;
class fields;
class weighted_flux_plane;

class flux_plane {
 public:
  double ymin, ymax, xconst;
  int is_rflux;
  int num_wf;
  double weights[2];
  int xpos[2];
  int verbosity;
  weighted_flux_plane *wf[2];
  flux_plane(double ymin, double ymax, double xconst, int is_rflux, double a);
  flux_plane(const flux_plane &fp);
  ~flux_plane();
  complex<double> flux(fields *f);
};

class monitor_point {
 public:
  monitor_point();
  monitor_point(double r, double z, const fields *f);
  ~monitor_point();
  vec loc;
  double t;
  complex<double> f[10];
  monitor_point *next;

  complex<double> get_component(component);

  // When called with only its first four arguments, fourier_transform
  // performs an FFT on its monitor points, putting the frequencies in f
  // and the amplitudes in a.  Yes, the frequencies are trivial and
  // redundant, but this saves you the risk of making a mistake in
  // converting your units.  Note also, that in this case f is always a
  // real number, although it's stored in a float.
  //
  // Note that in either case, fourier_transform assumes that the monitor
  // points are all equally spaced in time.
  void fourier_transform(component w,
                         complex<double> **a, complex<double> **f, int *numout,
                         double fmin=0.0, double fmax=0.0, int maxbands=100);
  // harminv works much like fourier_transform, except that it is not yet
  // implemented.
  void harminv(component w,
               complex<double> **a, double **f_re, double **f_im,
               int *numout, double fmin, double fmax,
               int maxbands);
};

class fields {
 public:
  double *(f[10][2]);
  double *(f_backup[10][2]);
  double *(f_pml[10][2]);
  double *(f_backup_pml[10][2]);

  double **(h_connection_sources[2]), **(h_connection_sinks[2]);
  int num_h_connections;
  complex<double> *h_phases;
  double **(e_connection_sources[2]), **(e_connection_sinks[2]);
  int num_e_connections;
  complex<double> *e_phases;

  polarization *pol, *olpol;
  double a, inva; // The "lattice constant" and its inverse!
  volume v;
  int m, t, phasein_time, is_real;
  double k, cosknz, sinknz;
  complex<double> eiknz;
  bandsdata *bands;
  src *e_sources, *h_sources;
  const mat *new_ma;
  mat *ma;
  const char *outdir;
  double preferred_fmax;

  fields(const mat *, int m=0);
  fields(const mat &, int m=0);
  void use_bloch(double kz);
  ~fields();

  void output_slices(const char *name = "");
  void output_slices(const volume &what, const char *name = "");
  void eps_slices(const char *name = "");
  void eps_slices(const volume &what, const char *name = "");
  void output_real_imaginary_slices(const char *name = "");
  void output_real_imaginary_slices(const volume &what, const char *name = "");
  void step();
  void step_right();
  inline double time() { return t*inva*c; };

  void use_real_fields();
  double find_last_source();
  void add_source(component whichf, double freq, double width, double peaktime,
                  double cutoff, complex<double> amp(const vec &),
                  int is_continuous = 0);
  void add_point_source(component whichf, double freq, double width, double peaktime,
                        double cutoff, const vec &, complex<double> amp = 1.0,
                        int is_continuous = 0);
  void add_plane_source(double freq, double width, double peaktime,
                        double cutoff, double envelope (const vec &),
                        const vec &p, const vec &norm = vec(0),
                        int is_continuous = 0);
  void initialize_field(component, complex<double> f(const vec &));
  void initialize_with_nth_te(int n);
  void initialize_with_nth_tm(int n);
  void initialize_with_n_te(int n);
  void initialize_with_n_tm(int n);
  void initialize_polarizations(polarization *op=NULL, polarization *np=NULL);
  int phase_in_material(const mat *ma, double time);
  int is_phasing();

  void get_point(monitor_point *p, const vec &);
  monitor_point *get_new_point(const vec &, monitor_point *p=NULL);
  void output_point(FILE *, const vec &, const char *name);

  flux_plane create_flux_plane(const vec &corner1, const vec &corner2);
  complex<double> get_flux(flux_plane *fp);
  
  void prepare_for_bands(const vec &, double end_time, double fmax=0,
                         double qmin=1e300, double frac_pow_min=0.0);
  void record_bands();
  complex<double> get_band(int n, int maxbands=100);
  void grace_bands(grace *, int maxbands=100);
  void output_bands(FILE *, const char *, int maxbands=100);
  void output_bands_and_modes(FILE *, const char *, int maxbands=100);
  double energy_in_box(const volume &);
  double electric_energy_in_box(const volume &);
  double magnetic_energy_in_box(const volume &);
  double thermo_energy_in_box(const volume &);
  double total_energy();
  double field_energy_in_box(const volume &);
  double field_energy();

  void set_output_directory(const char *name);
  void verbose(int v=1) { verbosity = v; }
 private: 
  int verbosity; // Turn on verbosity for debugging purposes...
  void phase_material();
  void step_h();
  void step_h_right();
  void step_h_boundaries();
  void step_h_source(const src *);
  void step_e();
  void step_e_right();
  void step_e_boundaries();
  void step_polarization_itself(polarization *old = NULL, polarization *newp = NULL);
  void step_e_polarization(polarization *old = NULL, polarization *newp = NULL);
  void step_e_source(const src *);
  void prepare_step_polarization_energy(polarization *op = NULL, polarization *np = NULL);
  void half_step_polarization_energy(polarization *op = NULL, polarization *np = NULL);
  void update_polarization_saturation(polarization *op = NULL, polarization *np = NULL);
  int cluster_some_bands_cleverly(double *tf, double *td, complex<double> *ta,
                                  int num_freqs, int fields_considered, int maxbands,
                                  complex<double> *fad, double *approx_power);
  void add_indexed_source(component whichf, double freq, double width,
                          double peaktime, int cutoff, int theindex, 
                          complex<double> amp, int is_c);
  void out_bands(FILE *, const char *, int maxbands);
  complex<double> *clever_cluster_bands(int maxbands, double *approx_power = NULL);
};

class grace_point;
enum grace_type { XY, ERROR_BARS };

class grace {
 public:
  grace(const char *fname, const char *dirname = ".");
  ~grace();
  
  void new_set(grace_type t = XY);
  void new_curve();
  void set_legend(const char *);
  void set_range(double xmin, double xmax, double ymin, double ymax);
  void output_point(double x, double y,
                    double dy = -1.0, double extra = -1.0);
  void output_out_of_order(int n, double x, double y,
                                  double dy = -1.0, double extra= -1.0);
 private:
  void flush_pts();
  FILE *f;
  char *fn, *dn;
  grace_point *pts;
  int set_num,sn;
};

// The following is a utility function to parse the executable name use it
// to come up with a directory name, avoiding overwriting any existing
// directory, unless the source file hasn't changed.

const char *make_output_directory(const char *exename, const char *jobname = NULL);
FILE *create_output_file(const char *dirname, const char *fname);

// The following allows you to hit ctrl-C to tell your calculation to stop
// and clean up.
void deal_with_ctrl_c(int stop_now = 2);
// When a ctrl_c is called, the following variable (which starts with a
// zero value) is incremented.
extern int interrupt;

int do_harminv(complex<double> *data, int n, int sampling_rate, double a,
	       double fmin, double fmax, int maxbands,
	       complex<double> *amps, double *freq_re, double *freq_im, double *errors = NULL);

#endif
