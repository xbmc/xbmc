<?xml version='1.0'?>
<xsl:stylesheet  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  version="1.0">
<!--  elements of customization for Xiph.org specs
      common to all docbook output formats

      this file is included by the format-specific stylesheets
      $Id: spec-common.xsl 7186 2004-07-20 07:19:25Z xiphmont $
-->

  <xsl:param name="use.svg" select="'0'"/>

  <xsl:param name="section.autolabel" select="'1'"/>
  <xsl:param name="section.label.includes.component.label" select="'1'"/>
  <xsl:param name="appendix.autolabel" select="'1'"/>

  <!-- we have latex versions of the equations inline -->
  <xsl:param name="tex.math.in.alt" select="'latex'"/>
  <xsl:param name="tex.math.delims" select="'0'"/>

  <!-- we're using xinclude instead to the extent that xsltproc doesn't support this -->
  <xsl:param name="textinclude.extension" select="'1'"/>

<!-- end common elements -->
</xsl:stylesheet>
