/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export default class Constants {
  //code and errorCode
  static CODE_RESULT = 1
  static CODE_CANCEL = 2
  static CODE_ACQUIRE = 3
  static ERROR_CODE_SUCCESS = 0
  static ERROR_CODE_FAIL = 1
  static ERROR_CODE_CANCEL = 2
  static ERROR_CODE_TIMEOUT = 4
  static ERROR_CODE_ALGORITHM_NOT_INIT = 5
  static ERROR_CODE_HAL_INVALID = 6
  static ERROR_CODE_OVER_MAX_FACES = 7
  static ERROR_CODE_INVALID_PARAMETERS = 9
  static ERROR_CODE_BUSY = 13
  static ERROR_CODE_ENROLL_HAS_REGISTERED = 16
  static ERROR_CODE_FACE_DETECTED = 38
  static ERROR_CODE_BAD_QUALITY = 4
  static ERROR_CODE_NOT_FOUND = 5
  static ERROR_CODE_TOO_SMALL = 6
  static ERROR_CODE_TOO_LARGE = 7
  static ERROR_CODE_OFFSET_LEFT = 8
  static ERROR_CODE_OFFSET_TOP = 9
  static ERROR_CODE_OFFSET_RIGHT = 10
  static ERROR_CODE_OFFSET_BOTTOM = 11
  static ERROR_CODE_LIVENESS_WARNING = 13
  static ERROR_CODE_LIVENESS_FAILURE = 14
  static ERROR_CODE_ROTATED_LEFT = 15
  static ERROR_CODE_RISE = 16
  static ERROR_CODE_ROTATED_RIGHT = 17
  static ERROR_CODE_DOWN = 18
  static ERROR_CODE_EYE_OCCLUSION = 21
  static ERROR_CODE_EYE_CLOSE = 22
  static ERROR_CODE_MOUTH_OCCLUSION = 23
  static ERROR_CODE_MULTI = 27
  static ERROR_CODE_BLUR = 28
  static ERROR_CODE_NOT_COMPLETE = 29
  static ERROR_CODE_DARKLIGHT = 30
  static ERROR_CODE_HIGHTLIGHT = 31
  static ERROR_CODE_HALF_SHADOW = 32
  static ERROR_CODE_HAS_REGISTERED = 37
  static ERROR_CODE_DARKPIC = 39
  static ERROR_CODE_ROTATE_TOP_RIGHT = 40
  static ERROR_CODE_ROTATE_TOP_LEFT = 41
  static ERROR_CODE_ROTATE_BOTTOM_RIGHT = 42
  static ERROR_CODE_ROTATE_BOTTOM_LEFT = 43
  static ERROR_CODE_OPEN_CAMERA_FAILED = 46
  static ERROR_CODE_PROGRESS_FIRST = 1004
  static ERROR_CODE_PROGRESS_SECOND = 1008
  static ERROR_CODE_PROGRESS_THIRD = 1012
  static PARAM_TOKEN = "check"
  static FLAGS = 0
  static FACEID = 0
  static PREVIEWID = 2

  //setTimeout
  static TIMEOUT_100 = 100
  static TIMEOUT_200 = 200
  static TIMEOUT_300 = 300
  static TIMEOUT_400 = 400
  static TIMEOUT_500 = 500
  static TIMEOUT_600 = 600
  static TIMEOUT_700 = 700
  static TIMEOUT_800 = 800
  static TIMEOUT_900 = 900
  static TIMEOUT_1000 = 1000
  static TIMEOUT_1100 = 1100
  static TIMEOUT_1200 = 1200
  static TIMEOUT_1300 = 1300
  static TIMEOUT_1400 = 1400
  static TIMEOUT_1500 = 1500
  static TIMEOUT_1600 = 1600
  static TIMEOUT_1700 = 1700
  static TIMEOUT_1800 = 1800
  static TIMEOUT_1900 = 1900
  static TIMEOUT_2000 = 2000
  static TIMEOUT_3000 = 3000

  //UI
  static SHELTER_ANIMATOR = 1000
  static PROGRESS_START = 0
  static PROGRESS_TOTAL = 100
  static FONT_WEIGHT = 500
  static SHELTER_START_WIDTH = 1100
  static SHELTER_END_WIDTH = '100%'
  static CENTER_HEIGHT = '620px'
  static SHELTER_START_HEIGHT = '2000px'
  static SHELTER_END_HEIGHT = '640px'
  static SHELTER_3D_END_HEIGHT = '640px'
  static PROGRESS_VALUE_FIRST = 30
  static PROGRESS_VALUE_SECOND = 60
  static PROGRESS_VALUE_THIRD = 100
  static SPACE = 20
  static INITIAL_INDEX = 0
  static HEIGHT_LIST = 60
  static FONTSIZE_16 = 16
  static BORDER_RADIUS = 10
  static STROKE_WIDTH = 2
  static START_MARGIN = 20
  static END_MARGIN = 20
  static SHOW_START_VIEWS_3D = '3D'
  static SHOW_START_VIEWS_2D = '2D'
  static LIST_PADDING = 5
  static BLUR_START = 0
  static BLUR_END = 40
  static CENTER_POSITION_Y = 5
  static BUTTON_POSITION = '92%'
  static ENROLL_INFO_POSITION = '918px'
  static ENROLL_INFO_WIDTH = '90%'
  static CHECKBOX_POSITION_X = '5%'
  static CHECKBOX_POSITION_Y = '85%'
  static CHECKBOX_TEXT_POSITION_X = '18%'
  static CHECKBOX_TEXT_POSITION_Y = '87%'

  static PROGRESS_WIDTH = '460px'
  static BACK_WIDTH = '136px'
  static BACK_HEIGHT = '110px'
  static TITLE_FONT_SIZE = '48px'
  static TITLE_MARGIN = '24px'
  static TITLE_ENROLLING_MARGIN = '140px'
  static IMAGE_MARGIN  = '96px'
  static IMAGE_3D_MARGIN = '48px'
  static FONT_SIZE = '28px'
  static SMALL_TITLE_FONT_SIZE = '36px'
  static CONTENT1_MARGIN = '16px'
  static CONTENT2_MARGIN = '8px'
  static BUTTON_FONT_SIZE = '32px'
  static BUTTON_WIDTH = '384px'
  static BUTTON_HEIGHT = '80px'
  static IMAGE_2D_HEIGHT = '480px'
  static IMAGE_3D_WIDTH = '512px'
  static ERROR_CODE_INFO_SIZE = '32px'
  static ENROLL_CENTER_POSITION_Y = '240px'
  static SUCCESS_WIDTH = '204px'
  static CHECKBOX_SIZE = '60px'
  static APERTURE1_WIDTH = '266px'
  static APERTURE2_WIDTH = '264px'
  static APERTURE34_WIDTH = '284px'
  static APERTURE5_WIDTH = '466px'

  //PROGRESS
  static PROGRESS_VALUE_5 = 5
  static PROGRESS_VALUE_10 = 10
  static PROGRESS_VALUE_15 = 15
  static PROGRESS_VALUE_20 = 20
  static PROGRESS_VALUE_25 = 25
  static PROGRESS_VALUE_30 = 30
  static PROGRESS_VALUE_35 = 35
  static PROGRESS_VALUE_40 = 40
  static PROGRESS_VALUE_45 = 45
  static PROGRESS_VALUE_50 = 50
  static PROGRESS_VALUE_55 = 55
  static PROGRESS_VALUE_60 = 60
  static PROGRESS_VALUE_65 = 65
  static PROGRESS_VALUE_70 = 70
  static PROGRESS_VALUE_75 = 75
  static PROGRESS_VALUE_80 = 80
  static PROGRESS_VALUE_85 = 85
  static PROGRESS_VALUE_90 = 90
  static PROGRESS_VALUE_95 = 95
  static PROGRESS_VALUE_100 = 100

  static DECLARE_WIDTH = '90%'
  static DECLARE_MAX_WIDTH = '100%'
  static DECLARE_TITLE_FONT_SIZE = 24
  static DECLARE_SECONDARY_TITLE_FONT_SIZE = 23
  static DECLARE_TEXT_FONT_SIZE = 22
  static DECLARE_ITEM_MARGIN_BOTTOM = 10
  static DECLARE_TITLE_MARGIN_TOP = 20
  static DECLARE_TITLE_MARGIN_BOTTOM = 20
  static DECLARE_TEXT_LINE_HEIGHT = 33
  static DECLARE_SCROLL_BAR_WIDTH = 0
  static DECLARE_BACKGROUND_COLOR = "#F2F3F5"
  static DECLARE_TITLE_2 = "2."
  static DECLARE_TITLE_3 = "3."
  static DECLARE_TITLE_4 = "4."
  static DECLARE_TITLE_5 = "5."
}