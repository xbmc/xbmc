# Set Option varname based on USE_INTERNAL_LIBS status
#
# Alternative to cmake_dependent_option
# cmake_dependent_option is restrictive, in the fact that we cannot override the 
# set option value as a cache variable (-Dvar=foo)
#
# This allows us to have the same outcome as cmake_dependent_option whilst still allowing
# user to override for platforms that would normally be forced ON
#
function(dependent_option varname optionmessage)

  # If varname already set, accept that, as it was provided by the user
  if(NOT DEFINED ${varname})
    # Generally we only define USE_INTERNAL_LIBS as the exception for platforms
    # we explicitly dont want to build internal libs (eg Linux/Freebsd)
    if(NOT DEFINED USE_INTERNAL_LIBS)
      option(${varname} ${optionmessage} ON)
    else()
      # Respect Value of USE_INTERNAL_LIBS for ON/OFF
      option(${varname} ${optionmessage} ${USE_INTERNAL_LIBS})
    endif()
  endif()
endfunction()
