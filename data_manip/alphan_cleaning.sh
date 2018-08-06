# change this to whatever your rat environment is
source ~/gitrat.sh

TESTING=0
EMIN=1
DELTA_T=1000000

# first remove the events that don't trigger
# production run 612
./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_2_TeDiol/Alphan_Telab_13c/*.root" /data/snoplus/OfficialProcessing/production_6_1_2_TeDiol/Alphan_Telab_13c_triggeredonly/trigs_only.root $TESTING

./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_13c/*.root" /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_13c_triggeredonly/trigs_only.root $TESTING

./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_18o/*.root" /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_18o_triggeredonly/trigs_only.root $TESTING

./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Ls_13c/*.root" /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Ls_13c_triggeredonly/trigs_only.root $TESTING

./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_13c/*.root" /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_13c_triggeredonly/trigs_only.root $TESTING

./remove_notrigs "/data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_18o/*.root" /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_18o_triggeredonly/trigs_only.root $TESTING


# production run 616
# now clean the tagged coiincidences
./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_2_TeDiol/Alphan_Telab_13c_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_2_TeDiol/Alphan_Telab_13c-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_13c $TESTING

./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_13c_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_13c-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_Avin_Av_13c $TESTING

./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_18o_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Av_18o-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_Avin_Av_18o $TESTING

./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Ls_13c_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avin_Ls_13c-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_Avin_Ls_13c $TESTING

./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_13c_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_13c-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_Avout_Av_13c $TESTING

./clean_alpha_n /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_18o_triggeredonly/trigs_only.root /data/snoplus/OfficialProcessing/production_6_1_6_TeDiol/Alphan_Telab_Avout_Av_18o-cleaned/cleaned.root $EMIN $DELTA_T /users/dunger/thesis_bb/data_manip/alphan_cut_eff/Alphan_Telab_Avout_Av_18o $TESTING


