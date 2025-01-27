// ----
// ---- file   : fx_example.h
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2024 by Bastian Spiegel.
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : tksampler voice plugin FX examples
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 21May2020, 24May2020, 25May2020
// ----          26May2020, 31May2020, 01Jun2020, 02Jun2020, 05Jun2020, 06Jun2020, 07Jun2020
// ----          08Jun2020, 09Jun2020, 08Feb2021, 11Feb2021, 23Feb2021, 14Apr2021, 02May2021
// ----          04May2021, 31May2021, 16Aug2021, 13Oct2021, 14Oct2021, 04Jan2023, 30Mar2023
// ----          27Sep2024, 14Oct2024, 08Nov2024
// ----
// ----
// ----

extern st_plugin_info_t *ringmod_init                  (void);
extern st_plugin_info_t *ws_tanh_init                  (void);
extern st_plugin_info_t *ws_tanh_asym_init             (void);
extern st_plugin_info_t *bitcrusher_init               (void);
extern st_plugin_info_t *ws_exp_init                   (void);
extern st_plugin_info_t *ws_fm_init                    (void);
extern st_plugin_info_t *resample_nearest_init         (void);
extern st_plugin_info_t *resample_linear_init          (void);
extern st_plugin_info_t *resample_tuned_init           (void);
extern st_plugin_info_t *biquad_lpf_1_init             (void);
extern st_plugin_info_t *biquad_lpf_2_init             (void);
extern st_plugin_info_t *biquad_lpf_3_init             (void);
extern st_plugin_info_t *biquad_lpf_4_init             (void);
extern st_plugin_info_t *biquad_hpf_1_init             (void);
extern st_plugin_info_t *biquad_hpf_2_init             (void);
extern st_plugin_info_t *biquad_hpf_3_init             (void);
extern st_plugin_info_t *biquad_hpf_4_init             (void);
extern st_plugin_info_t *biquad_bpf_1_init             (void);
extern st_plugin_info_t *biquad_bpf_2_init             (void);
extern st_plugin_info_t *biquad_bpf_3_init             (void);
extern st_plugin_info_t *biquad_bpf_4_init             (void);
extern st_plugin_info_t *biquad_brf_1_init             (void);
extern st_plugin_info_t *biquad_brf_2_init             (void);
extern st_plugin_info_t *biquad_brf_3_init             (void);
extern st_plugin_info_t *biquad_brf_4_init             (void);
extern st_plugin_info_t *biquad_peq_1_init             (void);
extern st_plugin_info_t *biquad_peq_2_init             (void);
extern st_plugin_info_t *biquad_peq_3_init             (void);
extern st_plugin_info_t *biquad_peq_4_init             (void);
extern st_plugin_info_t *biquad_lsh_1_init             (void);
extern st_plugin_info_t *biquad_lsh_2_init             (void);
extern st_plugin_info_t *biquad_lsh_3_init             (void);
extern st_plugin_info_t *biquad_lsh_4_init             (void);
extern st_plugin_info_t *biquad_hsh_1_init             (void);
extern st_plugin_info_t *biquad_hsh_2_init             (void);
extern st_plugin_info_t *biquad_hsh_3_init             (void);
extern st_plugin_info_t *biquad_hsh_4_init             (void);
extern st_plugin_info_t *biquad_vsf_1_init             (void);
extern st_plugin_info_t *biquad_vsf_2_init             (void);
extern st_plugin_info_t *biquad_vsf_3_init             (void);
extern st_plugin_info_t *biquad_vsf_4_init             (void);
extern st_plugin_info_t *amp_init                      (void);
extern st_plugin_info_t *pan_init                      (void);
extern st_plugin_info_t *ws_smoothstep_init            (void);
extern st_plugin_info_t *dly_1_init                    (void);
extern st_plugin_info_t *dly_2_init                    (void);
extern st_plugin_info_t *dly_flt_2_init                (void);
extern st_plugin_info_t *bitflipper_init               (void);
extern st_plugin_info_t *clip_init                     (void);
extern st_plugin_info_t *ringmul_init                  (void);
extern st_plugin_info_t *tuned_fb_init                 (void);
extern st_plugin_info_t *ladder_lpf_init               (void);
extern st_plugin_info_t *ladder_lpf_no_pan_init        (void);
extern st_plugin_info_t *ladder_lpf_x2_init            (void);
extern st_plugin_info_t *ladder_lpf_x2_no_pan_init     (void);
extern st_plugin_info_t *ladder_lpf_x3_init            (void);
extern st_plugin_info_t *ladder_lpf_x3_no_pan_init     (void);
extern st_plugin_info_t *ladder_lpf_x4_init            (void);
extern st_plugin_info_t *ladder_lpf_x4_no_pan_init     (void);
extern st_plugin_info_t *comp_rms_init                 (void);
extern st_plugin_info_t *ws_quintic_init               (void);
extern st_plugin_info_t *x_mix_init                    (void);
extern st_plugin_info_t *x_mul_init                    (void);
extern st_plugin_info_t *x_min_init                    (void);
extern st_plugin_info_t *x_max_init                    (void);
extern st_plugin_info_t *x_biquad_lpf_4_init           (void);
extern st_plugin_info_t *x_ladder_lpf_x4_init          (void);
extern st_plugin_info_t *x_mul_abs_init                (void);
extern st_plugin_info_t *abs_init                      (void);
extern st_plugin_info_t *rms_init                      (void);
extern st_plugin_info_t *ws_fold_wrap_init             (void);
extern st_plugin_info_t *ws_fold_wrap2_init            (void);
extern st_plugin_info_t *ws_fold_sine_init             (void);
extern st_plugin_info_t *dly_1_fade_init               (void);
extern st_plugin_info_t *dly_2_fade_init               (void);
extern st_plugin_info_t *dly_flt_2_fade_init           (void);
extern st_plugin_info_t *ws_flex_init                  (void);
extern st_plugin_info_t *ws_flex_asym_init             (void);
extern st_plugin_info_t *ws_slew_init                  (void);
extern st_plugin_info_t *ws_slew_asym_init             (void);
extern st_plugin_info_t *ws_sin_exp_init               (void);
extern st_plugin_info_t *boost_init                    (void);
extern st_plugin_info_t *eq3_init                      (void);
extern st_plugin_info_t *ws_slew_8bit_init             (void);
extern st_plugin_info_t *wave_multiplier_wrap_init     (void);
extern st_plugin_info_t *wave_multiplier_reflect_init  (void);
extern st_plugin_info_t *wave_multiplier_delay_init    (void);
extern st_plugin_info_t *wave_multiplier_delay4_init   (void);
extern st_plugin_info_t *wave_multiplier_delay8_init   (void);
extern st_plugin_info_t *wave_multiplier_delay16_init  (void);
extern st_plugin_info_t *wave_multiplier_allpass4_init (void);
extern st_plugin_info_t *wave_multiplier_allpass8_init (void);
extern st_plugin_info_t *wave_multiplier_apdly4_init   (void);
extern st_plugin_info_t *wave_multiplier_apdly8_init   (void);
extern st_plugin_info_t *fold_init                     (void);
extern st_plugin_info_t *modfm_init                    (void);
extern st_plugin_info_t *dly_flt_2_mod_init            (void);
extern st_plugin_info_t *ws_lin_cpx_init               (void);
