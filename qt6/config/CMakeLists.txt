if(NOT QTC_QT6_ENABLE_KDE)
  return()
endif()

set(kstyle_qtcurve5_config_PART_SRCS
  qtcurveconfig.cpp
  imagepropertiesdialog.cpp
  ../common/common.cpp
  ../common/config_file.cpp
  ../common/kf5_utils.cpp
  ../kwinconfig/qtcurvekwinconfig.cpp
  ../kwin/qtcurveconfig.cpp
  ../kwin/qtcurveshadowconfiguration.cpp)

set(kstyle_qtcurve5_config_export_SRCS
  exportthemedialog.cpp)

set(kstyle_qtcurve5_config_PART_HDRS
  qtcurveconfig.h
  imagepropertiesdialog.h)

set(kstyle_qtcurve5_config_export_HDRS
  exportthemedialog.h)

set(kstyle_qtcurve5_config_UIS
  qtcurveconfigbase.ui
  stylepreview.ui
  imageproperties.ui
  ../kwinconfig/qtcurvekwinconfigwidget.ui)

if(QTC_QT6_STYLE_SUPPORT)
  set(kstyle_qtcurve5_config_PART_SRCS
    ${kstyle_qtcurve5_config_PART_SRCS}
    ${kstyle_qtcurve5_config_export_SRCS})
endif()

translate_add_sources(
  ${kstyle_qtcurve5_config_PART_SRCS}
  ${kstyle_qtcurve5_config_export_SRCS}
  ${kstyle_qtcurve5_config_PART_HDRS}
  ${kstyle_qtcurve5_config_export_HDRS}
  ${kstyle_qtcurve5_config_UIS})

add_definitions(-DCONFIG_WRITE -DCONFIG_DIALOG)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

ki18n_wrap_ui(kstyle_qtcurve5_config_PART_SRCS ${kstyle_qtcurve5_config_UIS})

add_library(kstyle_qtcurve5_config MODULE ${kstyle_qtcurve5_config_PART_SRCS})

target_link_libraries(kstyle_qtcurve5_config Qt6::Core Qt6::Gui Qt6::Widgets)
target_link_libraries(kstyle_qtcurve5_config
  KF5::Archive
  KF5::ConfigCore
  KF5::ConfigWidgets
  KF5::I18n
  KF5::GuiAddons
  KF5::IconThemes
  KF5::KIOWidgets
  KF5::WidgetsAddons
  KF5::XmlGui
  KF5::Style
  KF5::WindowSystem)
target_link_libraries(kstyle_qtcurve5_config qtcurve-utils)

install(TARGETS kstyle_qtcurve5_config DESTINATION ${_Qt6_PLUGIN_INSTALL_DIR})
install(FILES QtCurveui.rc  DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/QtCurve)
