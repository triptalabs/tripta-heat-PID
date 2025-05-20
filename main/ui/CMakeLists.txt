SET(SOURCES components/ui_comp_statusbar.c
    components/ui_comp.c
    screens/ui_ScreenBootlogo.c
    screens/ui_ScreenHome.c
    screens/ui_ScreenBT.c
    screens/ui_ScreenAjustes.c
    screens/ui_ScreenWifi.c
    screens/ui_ScreenEstadisticas.c
    screens/ui_Devmode.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_logo_blanco_png_png.c
    images/ui_img_iconupdate_png.c
    images/ui_img_iconheating_png.c
    images/ui_img_iconbt_png.c
    images/ui_img_iconwifi_png.c
    images/ui_img_iconwarn_png.c
    images/ui_img_img_cooling_png.c
    images/ui_img_img_heating2_png.c
    images/ui_img_buttonminus_png.c
    images/ui_img_buttonminuspressed_png.c
    images/ui_img_buttonplus_1_png.c
    images/ui_img_buttonpluspressed_png.c
    images/ui_img_iconsettings_png.c
    images/ui_img_iconstatistic_png.c
    images/ui_img_logoblanco_png.c
    images/ui_img_img_bt_png.c
    images/ui_img_arrowicon_1_png.c
    images/ui_img_qrcode_png.c
    images/ui_img_imgsettings_png.c
    images/ui_img_img_wifi_png.c)

add_library(ui ${SOURCES})
