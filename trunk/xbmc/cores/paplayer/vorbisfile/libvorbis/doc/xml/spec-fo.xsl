<?xml version='1.0'?> 
<xsl:stylesheet  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  version="1.0"> 
  <xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/> 
  <xsl:include href="spec-common.xsl"/>
  <xsl:param name="paper.type" select="'A4'"/> 

  <xsl:attribute-set name="section.title.level1.properties">
    <xsl:attribute name="break-before">page</xsl:attribute>
  </xsl:attribute-set>

  <!-- the appendix pagebreak setting doesn't seem to be respected -->
  <xsl:attribute-set name="appendix.title.properties">
    <xsl:attribute name="break-before">page</xsl:attribute>
  </xsl:attribute-set>

</xsl:stylesheet>  
