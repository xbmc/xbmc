<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" />

	<xsl:template match="/ports">
		<html>
			<head>
				<title>ASAP ports list</title>
				<style>
					table { border-collapse: collapse; }
					th, td { border: solid black 1px; }
					th, .name { background-color: #ccf; }
					.yes { background-color: #cfc; }
					.no { background-color: #fcc; }
					.partial { background-color: #ffc; }
					.yes, .no, .partial { text-align: center; }
				</style>
			</head>
			<body>
				<table>
					<tr>
						<th>Name</th>
						<th>Binary release</th>
						<th>Platform</th>
						<th>User interface</th>
						<th>First appeared in&#160;ASAP</th>
						<th>Development status</th>
						<th>Output</th>
						<th>Supports subsongs?</th>
						<th>Shows file information?</th>
						<th>Edits file information?</th>
						<th>Converts to and from SAP?</th>
						<th>Configurable playback time?</th>
						<th>Mute POKEY channels?</th>
						<th>Programming language</th>
						<th>Related website</th>
					</tr>
					<xsl:apply-templates />
				</table>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="port">
		<tr>
			<td class="name"><xsl:value-of select="@name" /></td>
			<xsl:apply-templates />
			<td><xsl:copy-of select="a" /></td>
		</tr>
	</xsl:template>

	<xsl:template match="bin|platform|interface|since|output|lang">
		<td>
			<xsl:value-of select="." />
		</td>
	</xsl:template>

	<xsl:template match="status">
		<td>
			<xsl:attribute name="class">
				<xsl:choose>
					<xsl:when test=". = 'stable'">yes</xsl:when>
					<xsl:when test=". = 'experimental'">no</xsl:when>
					<xsl:otherwise>partial</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>
			<xsl:value-of select="." />
		</td>
	</xsl:template>

	<xsl:template match="subsongs|file-info|edit-info|convert-sap|config-time|mute-pokey">
		<td>
			<xsl:attribute name="class">
				<xsl:choose>
					<xsl:when test=". = 'yes' or . = 'no'">
						<xsl:value-of select="." />
					</xsl:when>
					<xsl:otherwise>partial</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>
			<xsl:value-of select="." />
		</td>
	</xsl:template>

	<xsl:template match="a" />
</xsl:stylesheet>
