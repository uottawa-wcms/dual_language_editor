var dual_language_editor_in_motion = false;
var dual_language_editor_button_state = 'center';
(function($) {
  $(document).ready(function() {
    $('.collapse').click(function() {
      if (!dual_language_editor_in_motion) {
        var shrinkTarget = '';
        var expandTarget = '';
        if ($(this).attr('id') == 'collapse-right') {
          shrinkTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(2)';
          expandTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(1)';
        } else {
          shrinkTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(1)';
          expandTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(2)';
        }
        dual_language_editor_in_motion = true;
        dual_language_editor_button_state = 'center';

        $(shrinkTarget).animate({
          width: '49%'
        }, 250, null, function() {
          $(expandTarget).css('width', '49%');
          $(expandTarget).css('display', 'block');
          $(expandTarget).animate({
            opacity: 1
          }, 250, null, function() {
            updateButtonState2();
          })
        });
      }
    });
     $('.expand').click(function() {
        if (!dual_language_editor_in_motion) {
          var shrinkTarget = '';
          var expandTarget = '';
          if ($(this).attr('id') == 'expand-right') {
            shrinkTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(1)';
            expandTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(2)';
            dual_language_editor_button_state = 'right';
          } else {
            shrinkTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(2)';
            expandTarget = 'div.dual-language-editor-row > div.form-wrapper > div:nth-of-type(1)';
            dual_language_editor_button_state = 'left';
          }
          dual_language_editor_in_motion = true;
          $(shrinkTarget).animate({
            opacity: 0
          }, 300, function() {
            $(shrinkTarget).css('display', 'none');
            $(expandTarget).animate({
              width: '100%'
            }, 250, null, function() {
              updateButtonState2();
            });
          });
        }
    });
  });

}(jQuery));



function updateButtonState2() {
  if (dual_language_editor_in_motion) {
    (function($) {
      dual_language_editor_in_motion = false;
      $('.collapse').css('display', 'none');
      $('.expand').css('display', 'none');
      switch (dual_language_editor_button_state) {
        case 'center':
          $('.expand').css('display', 'inline');
          break;
        case 'left':
          $('#collapse-left').css('display', 'inline');
          break;
        case 'right':
          $('#collapse-right').css('display', 'inline');
          break;
      }
    }(jQuery));
    return true;
  }
}