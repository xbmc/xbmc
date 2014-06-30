/**
 * MODIFIED CAUSE WE NEEDED OUR OWN MARKUP
 * stacktable.js
 * Author & copyright (c) 2012: John Polacek
 * Dual MIT & GPL license
 *
 * Page: http://johnpolacek.github.com/stacktable.js
 * Repo: https://github.com/johnpolacek/stacktable.js/
 *
 * jQuery plugin for stacking tables on small screens
 *
 */
;(function($) {

  $.fn.stacktable = function(options) {
    var $tables = this,
        defaults = {id:'stacktable',hideOriginal:false},
        settings = $.extend({}, defaults, options),
        stacktable;

    return $tables.each(function() {
      var $stacktable = $('<table class="'+settings.id+'"><tbody></tbody></table>');
      if (typeof settings.myClass !== undefined) $stacktable.addClass(settings.myClass);
      var markup = '';
      $table = $(this);
      $topRow = $table.find('tr').eq(0);
      $table.find('tr').each(function(index,value) {
        var zebra = "";
        if (index % 2 === 0) {
          zebra = "even";
        } else {
          zebra = "odd";
        }
        markup += '<tr class="' + zebra + '">';
        $(this).find('td').each(function(index,value) {
          if ($(this).html() !== ''){
            markup += '<tr class="' + zebra + '">';
            if ($topRow.find('td,th').eq(index).html()){
              markup += '<td>'+$topRow.find('td,th').eq(index).html()+'</td>';
            } else {
              markup += '<td></td>';
            }
            markup += '<td>'+$(this).html()+'</td>';
            markup += '</tr>';
          }
        });
      });
      $stacktable.append($(markup));
      $table.before($stacktable);
      if (settings.hideOriginal) $table.hide();
    });
  };

}(jQuery));
