///////////////////////////////////////////////////////////////////////////////
// ORACLE PRO*FORTRAN SQL converter to PROC*C
//
//
//  To generate the Sql/C embeded codes you must install the f2c package
//  To compile in Oracle the PRO*C is needed
//  The C program generated can be easily ported to other DB plataforms
//
//
// Autor: Andre Vinicius Pereira de Rezende
// Email: avini@embratel.com.br
// Data:  05/05/2002
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
///////////////////////////////////////////////////////////////////////////////
void CheckArgs(int argc,char **argv);
void PrepareProFortranFile(void);
void ConvertProFortranFile2ProC(void);
FILE *OpenFile(const char *path, const char *mode,int priority);
char *IgnoreExtension(char *szFileName);
char *strlower(char *string);
int isExecSqlStatement(char *line);
char* AdjustTokens(char* szstring);
void MakeExecutable(void);
char* SupressNewLine(char* szLine);
static int IdSqlDeclareSection;
char szSubRoutineContext[50];
char *SupressNonValidCharacters(char *szLine);
void AppendInFortranVariableList(char *szNomeVar,char *szTypeVar,int iSize,char *szContext);
void	ChangeExecSqlDeclarePosition2BeforeFormat(void);
void ReportFortranSqlVars(void);
char *GetDeclarationItemTokenByIndex(int index,char *szLine);
char *GetSqlTokenByIndex(int index,char *szLine);
char *GetTokByIndex(int index,char *szBuffer,char *separator);
///////////////////////////////////////////////////////////////////////////////

char sz_proforSqlTraceOutput[1024];
int iSqlLogMode;
#define MAX_LINE 4096
#define DEF_ConvFortranChar2C \
"char* ConvFortranChar2C(char* szstring,int size) \n\
{ \n\
	 int i=size-1; \n\
	szstring[size]=0x00; \n\
	while(i>0) \n\
	{ \n\
	   if(szstring[i]>=0x21 && szstring[i]<=0x7e) \n\
		   break; \n\
						szstring[i]=0x00; \n\
						i=i--; \n\
			} \n\
			return(szstring);\n\
} \n\n"

#define DEF_ConvC2FortranChar \
"char* ConvC2FortranChar(char* szstring,int size) \n\
{ \n\
	 int i=size-1; \n\
		while(i>0) \n\
		{ \n\
		   if(szstring[i]<0x21 || szstring[i]>0x7e) \n\
						  szstring[i]=' '; \n\
						i=i--; \n\
			} \n\
			return(szstring);\n\
} \n\n"

#define DEF_proforSqlTraceOutput \
"void _proforSqlTraceOutput(char* szstring) \n\
{ \n\
	 FILE *fout; \n\
		_proforIsNewSqlTraceOutput(); \n\
  fout = fopen(DEF_SQLLOGPROFOROUTPUT,\"a\"); \n\
		if(fout==NULL) \n\
		  return; \n\
		fputs(DEF_PROFORSOURCE,fout); \n\
		fputs(\":     \",fout); \n\
		fputs(szstring,fout); \n\
		fputs(\"\\n\",fout); \n\
		fclose(fout);	\n\
} \n\n"

#define DEF_proforSqlTraceSQLCA \
"void _proforSqlTraceSQLCA(void) \n\
{ \n\
	 FILE *fout; \n\
		if (sqlca.sqlcode >= 0) \n\
		  return; \n\
		_proforIsNewSqlTraceOutput(); \n\
  fout = fopen(DEF_SQLLOGPROFOROUTPUT,\"a\"); \n\
		if(fout==NULL) \n\
		  return; \n\
	 fprintf(fout,\"%s:     Last SQL message: %s\",DEF_PROFORSOURCE,sqlca.sqlerrm.sqlerrmc); \n\
		fputs(\"\\n\",fout); \n\
		fclose(fout);	\n\
} \n\n"

#define DEF_proforSqlTraceOutputVarDouble \
"void _proforSqlTraceOutputVarDouble(char* szstring,double dvar) \n\
{ \n\
	 FILE *fout; \n\
		_proforIsNewSqlTraceOutput(); \n\
  fout = fopen(DEF_SQLLOGPROFOROUTPUT,\"a\"); \n\
		if(fout==NULL) \n\
		  return; \n\
		fprintf(fout,\"%s:     %s = %f (double)\",DEF_PROFORSOURCE,szstring,dvar); \n\
		fputs(\"\\n\",fout); \n\
		fclose(fout);	\n\
} \n\n"

#define DEF_proforSqlTraceOutputVarInteger \
"void _proforSqlTraceOutputVarInteger(char* szstring,int dvar) \n\
{ \n\
	 FILE *fout; \n\
		_proforIsNewSqlTraceOutput(); \n\
  fout = fopen(DEF_SQLLOGPROFOROUTPUT,\"a\"); \n\
		if(fout==NULL) \n\
		  return; \n\
		fprintf(fout,\"%s:     %s = %d (int)\",DEF_PROFORSOURCE,szstring,dvar); \n\
		fputs(\"\\n\",fout); \n\
		fclose(fout);	\n\
} \n\n"

#define DEF_proforSqlTraceOutputVarChar \
"void _proforSqlTraceOutputVarChar(char* szstring,char *dvar) \n\
{ \n\
	 FILE *fout; \n\
		_proforIsNewSqlTraceOutput(); \n\
  fout = fopen(DEF_SQLLOGPROFOROUTPUT,\"a\"); \n\
		if(fout==NULL) \n\
		  return; \n\
		fprintf(fout,\"%s:     %s = [%s] (char *)\",DEF_PROFORSOURCE,szstring,dvar); \n\
		fputs(\"\\n\",fout); \n\
		fclose(fout);	\n\
} \n\n"

#define DEF_proforIsNewSqlTraceOutput \
"static int IsNewSqlTraceOutputFlag; \n\
void _proforIsNewSqlTraceOutput(void) \n\
{ \n\
	 char szCmd[1024]; \n\
		if(!IsNewSqlTraceOutputFlag) \n\
		{ \n\
		  strcpy(szCmd,\"rm -rf \"); strcat(szCmd,DEF_SQLLOGPROFOROUTPUT); \n\
				system(szCmd); \n\
				IsNewSqlTraceOutputFlag=1; \n\
		  return; \n\
		} \n\
		return; \n\
} \n\n"

int iLineAtual;
char szAtualFile[512];
int iDebugMode;

char szFileProFor[512];
char szFileProForAux[512];
char szFileFortran[512];
char szFileC[512];
char szFileProC[512];
char szLine[(MAX_LINE)+1];
char szLineAux[(MAX_LINE)+1];

char szLineBuf[(MAX_LINE)+1];
char szLineBuf1[(MAX_LINE)+1];
///////////////////////////////////////////////////////////////////////////////
#define DEF_MAX_VAR_WORK 20000
struct {
   char name[50];
			char type[20];
			int size;
			int ischangedname;
			char context[50];   
} stListaVariables[DEF_MAX_VAR_WORK];
int iEndVariableList;
///////////////////////////////////////////////////////////////////////////////
struct {
   char name[50];
			char commonname[50];   
			char structname[100];   
			char context[50];   
} stListaCommonVariables[DEF_MAX_VAR_WORK];
static char szGlobalCommonName[50];
int iEndCommonVariableList;
///////////////////////////////////////////////////////////////////////////////
struct {
   char name[50];
} stListaChangePositionFlag[DEF_MAX_VAR_WORK];
int iEndListaChangePositionFlag;
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlTraceSourceLine(char *szline)
{
    static char szBuf[(MAX_LINE)+1];
				szBuf[0]=0x00;
    if(strstr(szLine,"EXEC SQL INCLUDE SQLCA;")==NULL)
				  sprintf(szBuf,"_proforSqlTraceOutput(\"%s\");\n",szLine);				
				return(szBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlTraceSQLCA(char *szline)
{
    static char szBuf[(MAX_LINE)+1];
				szBuf[0]=0x00;
    if(strstr(szLine,"EXEC SQL INCLUDE SQLCA;")==NULL)
				  sprintf(szBuf,"_proforSqlTraceSQLCA();\n");				
				return(szBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlLogVarDoubleOutput(char *szvar)
{
    static char szBuf[(MAX_LINE)+1];
				szBuf[0]=0x00;
				strcpy(szBuf,"_proforSqlTraceOutputVarDouble(\"");	strcat(szBuf,szvar);		
				strcat(szBuf,"\",(double)");		strcat(szBuf,szvar); strcat(szBuf,");\n");
				return(szBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlLogVarIntegerOutput(char *szvar)
{
    static char szBuf[(MAX_LINE)+1];
				szBuf[0]=0x00;
				strcpy(szBuf,"_proforSqlTraceOutputVarInteger(\"");	strcat(szBuf,szvar);		
				strcat(szBuf,"\",(int)");		strcat(szBuf,szvar); strcat(szBuf,");\n");
				return(szBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlLogVarCharOutput(char *szvar)
{
    static char szBuf[(MAX_LINE)+1];
				szBuf[0]=0x00;
				strcpy(szBuf,"_proforSqlTraceOutputVarChar(\"");	strcat(szBuf,szvar);		
				strcat(szBuf,"\",(char *)");		strcat(szBuf,szvar); strcat(szBuf,");\n");
				return(szBuf);
}
///////////////////////////////////////////////////////////////////////////////
// Programa Principal
int main(int argc,char **argv)
{
  iDebugMode=0;
  IdSqlDeclareSection=0;
		strcpy(szSubRoutineContext,"____GLOBALCONTEXT____");
		iEndVariableList=0;
  CheckArgs(argc,argv);
  PrepareProFortranFile();
		ConvertProFortranFile2ProC();
		ChangeExecSqlDeclarePosition2BeforeFormat();
		MakeExecutable();
		ReportFortranSqlVars();
  return(0);
}
///////////////////////////////////////////////////////////////////////////////
int IsDeclareSqlGlobalFortranVar(char *szNomeVar)
{
   int iSearch;
			char szBuffTemp[50];
			strcpy(szBuffTemp,szNomeVar);
			strcat(szBuffTemp,"_f2c");
   iSearch=0;
   while(iSearch<iEndVariableList)
			{
			   if(strcmp(stListaVariables[iSearch].name,szBuffTemp)==0 )
						{
						  return(1);
						}		
      iSearch++;
			}
			return(0);
}

///////////////////////////////////////////////////////////////////////////////
char* GetSqlGlobalFortranVarStruct(char *szNomeVar)
{
   int iSearch;
			char szBuffTemp[50];

			strcpy(szBuffTemp,szNomeVar);
   iSearch=0;
   while(iSearch<iEndCommonVariableList)
			{
			   if(strcmp(stListaCommonVariables[iSearch].name,szBuffTemp)==0 )
						{
		       strcpy(szNomeVar,stListaCommonVariables[iSearch].structname); 
						   return(szNomeVar);
						}		
      iSearch++;
			}
			return(szNomeVar); // Se a variavel nao ´e global retorna ela mesma.
}
///////////////////////////////////////////////////////////////////////////////
void ReportFortranSqlVars(void)
{
   int i,iFlag,iWarning;
   char szFlag[2];
			i=0;
			iWarning=0;
   fprintf(stdout,"+------------------------------------------------------------------------+\n");
   fprintf(stdout,"| Variaveis com acesso ao banco (DECLARE SECTION)                        |\n");
   fprintf(stdout,"+----------------------+-----------+--------+----------+-----------------+\n");
   fprintf(stdout,"| Var. Name            | Type      | Size   | B.Change | Context         |\n");			
   fprintf(stdout,"+----------------------+-----------+--------+----------+-----------------+\n");
			while(i<iEndVariableList)
			{
      fprintf(stdout,"| %20s | %9s ",stListaVariables[i].name,stListaVariables[i].type);			
						fprintf(stdout,"| %6d | %8d ",stListaVariables[i].size,stListaVariables[i].ischangedname);			
						fprintf(stdout,"| %15s |\n"   ,stListaVariables[i].context);			
						i++;
			}
   fprintf(stdout,"+----------------------+-----------+--------+----------+-----------------+\n\n");
   i=0;
   fprintf(stdout,"+------------------------------------------------------------------------------------+\n");
   fprintf(stdout,"| Variaveis COMMON                                                                   |\n");
   fprintf(stdout,"+----------------------+-----------------+-----+-------------------------------------+\n");
   fprintf(stdout,"| Var. Name            | Common Context  | SQL | Struct Name                         |\n");			
   fprintf(stdout,"+----------------------+-----------------+-----+-------------------------------------+\n");
			while(i<iEndCommonVariableList)
			{
	     iFlag = IsDeclareSqlGlobalFortranVar(stListaCommonVariables[i].name);

						if(iFlag==1)
						{
						  strcpy(szFlag,"*");
						  iWarning=1;
						}  
						else
						  strcpy(szFlag," ");	
      fprintf(stdout,"| %20s | %15s |",stListaCommonVariables[i].name,stListaCommonVariables[i].commonname);			
      fprintf(stdout," %3s | %35s |\n",szFlag,stListaCommonVariables[i].structname);			
						i++;
			}
   fprintf(stdout,"+----------------------+-----------------+-----+-------------------------------------+\n");
//			if(iWarning)
//			{
//			   fprintf(stdout,"\n\n\n\n");
//			   fprintf(stdout,"ATENCAO - Foram encontradas variaveis do  tipo char e COMMON com  acesso ao banco\n");
//			   fprintf(stdout,"          estas variaveis devem ser retiradas para evitar bugs na execuçao.\n");
//			   fprintf(stdout,"\n\n\n\n");
//						exit(1);
//   } 
			
}
///////////////////////////////////////////////////////////////////////////////
void MsgDebugLine(char *line)
{
   char szBuffer[(MAX_LINE)+1];
   if(iDebugMode)
			{
			   strcpy(szBuffer,line);
      fprintf(stdout,"%s:%d:[%s]\n",szAtualFile,iLineAtual,SupressNewLine(szBuffer));
			}		
			iLineAtual++;
}
///////////////////////////////////////////////////////////////////////////////
void MakeExecutable(void)
{
   char szCmdLine[(MAX_LINE)+1];
		
		 sprintf(szCmdLine,"rm -rf %s",szFileC);
		 system(szCmdLine);
		 sprintf(szCmdLine,"rm -rf %s",szFileFortran);
   system(szCmdLine);
		 sprintf(szCmdLine,"rm -rf %s",szFileProForAux);
   system(szCmdLine);
		 sprintf(szCmdLine,"rm -rf %s",IgnoreExtension(szFileFortran));
   system(szCmdLine);
		
//		fprintf(stdout,"Gerando o executavel [%s]\n",IgnoreExtension(szFileFortran));
//  fprintf(stdout,"-----------------------------------------------------------------------\n"); 
//		strcpy(szCmdLine,"make ");
//		strcat(szCmdLine,IgnoreExtension(szFileFortran));
//		system(szCmdLine);
//		fprintf(stdout,"Fim\n");
//  fprintf(stdout,"-----------------------------------------------------------------------\n");
}
///////////////////////////////////////////////////////////////////////////////
char* AdjustTokens(char* szstring)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,szstring);
   szstring[0]=0x00;
   buf=strtok(szLineBuf, " ");
			while(buf!=NULL)
			{
		    strcat(szstring,buf);	
			   buf=strtok(NULL, " ");
						if(buf!=NULL)
						  strcat(szstring," ");	
			}
			return(szstring);
}
///////////////////////////////////////////////////////////////////////////////
int isPreprocessorComment(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   buf=strstr(szLineBuf, "//");
   if(buf!=NULL)
     buf[0]=0x00; // Ignora o conteudo da linha apos o "//"
     
			buf = strtok(szLineBuf, " "); if(buf==NULL) return(0);
   if(strcmp(buf,"/*")==0)
			{
	    buf = strtok(NULL, " "); if(buf==NULL) return(0);
     if(strcmp(buf,"__PREPROC__")==0)
       return(1);
			}				
   return(0);
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Rotinas para implementaçao do codigo C inline no FORTRAN
int isPreprocessorInlineComment(char *line)
{
   if( strcmp(GetTokByIndex(0,line," "),"/*") == 0             && 
			    strcmp(GetTokByIndex(1,line," "),"PREPROC_INLINE") == 0 
      )
      return(1);
			else
      return(0);
}
///////////////////////////////////////////////////////////////////////////////
char* SupressPreprocessorInlineComment(char* szLine)
{
  char* buf;
		buf = strstr(szLine,"*/");
		if(buf!=NULL)
		   buf[0]=0x00;
		
		szLineBuf[0]=0x00;
		strcpy(szLine,GetTokByIndex(2,szLine," "));
		strcat(szLine,"\n");
  return szLine;
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
char* SupressPreprocessorComment(char* szLine)
{
  char* buf;
		buf = strstr(szLine,"*/");
		if(buf!=NULL)
		   buf[0]=0x00;
		strcpy(szLine,&szLine[14]);
		strcat(szLine,";\n");
  return szLine;
}
///////////////////////////////////////////////////////////////////////////////
char *GetTokByIndex(int index,char *szBuffer,char *separator)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf; int ind;
   strcpy(szLineBuf,szBuffer);
   
			ind=0;  
			buf = strtok(szLineBuf, separator); 
			if(ind==index) return(buf);
			while(buf!=NULL)
			{
			   if(ind==index) return(buf);
	     buf = strtok(NULL, separator); 
						ind++;	
			}
			return(buf);
}
///////////////////////////////////////////////////////////////////////////////
int isFortranVariableDeclaration(char *szLine)
{
    static char szLineBuf[(MAX_LINE)+1];
				if(szLine==NULL)
				  return(0);
				strcpy(szLineBuf,szLine);
    strlower(szLineBuf);
				
				if(strstr(GetTokByIndex(2,szLineBuf," "),"character")!=NULL)
				  return(1);
				if(strstr(GetTokByIndex(2,szLineBuf," "),"integer")!=NULL)
				  return(1);
				if(strstr(GetTokByIndex(2,szLineBuf," "),"real")!=NULL)
				  return(1);
				if(strstr(GetTokByIndex(2,szLineBuf," "),"logical")!=NULL)
				  return(1);
    return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isVariableDeclaration(char *szLine)
{
   char *buf;
   static char szLineBuf[(MAX_LINE)+1];
			strcpy(szLineBuf,szLine);
			strlower(szLineBuf);
   buf = GetTokByIndex(2,szLineBuf," ");
			if(buf==NULL)
			  return(0);
			if(isFortranVariableDeclaration(szLineBuf))
			  return (1);	
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
char *AdjustVariableDeclaration(char *szLine)
{
   char *buf;
   static char szLineBuf[(MAX_LINE)+1];
   char szSizeChar[10];
			char *size;
			strcpy(szLineBuf,szLine);
			strlower(szLineBuf);
   buf = GetTokByIndex(0,szLineBuf," ");
   if(strstr(buf,"character")!=NULL)
			{
			   buf = strstr(buf,"*");
						if(buf!=NULL)
			     strcpy(szSizeChar,&buf[1]);
						else
			     strcpy(szSizeChar,"1");
			   sprintf(szLine,"              static char %s[%d];\n",GetTokByIndex(1,szLineBuf," "),atoi(szSizeChar)+1);
						AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"char",atoi(szSizeChar)+1,szSubRoutineContext);
						return(szLine);
			}
   if(strstr(buf,"integer")!=NULL)
			{
			   buf = strstr(buf,"*");
						if(buf!=NULL)
			     strcpy(szSizeChar,&buf[1]);
						else
			     strcpy(szSizeChar,"4");
			   sprintf(szLine,"              static int %s;\n",GetTokByIndex(1,szLineBuf," "));
					 AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"int",atoi(szSizeChar),szSubRoutineContext);
						
					//	if(atoi(szSizeChar)==8)
					//	{
			  //    sprintf(szLine,"              long int %s;\n",GetTokByIndex(1,szLineBuf," "));
					//	   AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"int",atoi(szSizeChar),szSubRoutineContext);
					//	}
					//	else
					//	{
			  //    sprintf(szLine,"              int %s;\n",GetTokByIndex(1,szLineBuf," "));
					//    AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"int",atoi(szSizeChar),szSubRoutineContext);
					//	}
						return(szLine);
			}

   if(strstr(buf,"real")!=NULL)
			{
			   buf = strstr(buf,"*");
						if(buf!=NULL)
			     strcpy(szSizeChar,&buf[1]);
						else
			     strcpy(szSizeChar,"4");
						if(atoi(szSizeChar)==4)
						{
			   			sprintf(szLine,"              static float %s;\n",GetTokByIndex(1,szLineBuf," "));
									AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"float",atoi(szSizeChar),szSubRoutineContext);
						}
						else
						{
			   			sprintf(szLine,"              static double %s;\n",GetTokByIndex(1,szLineBuf," "));
									AppendInFortranVariableList(GetTokByIndex(1,szLineBuf," "),"double",atoi(szSizeChar),szSubRoutineContext);
						}
						
						return(szLine);
			}
			fprintf(stdout,"[AdjustVariableDeclaration] Linha [%d] - Variavel FORTRAN: [%s] Context: [%s] nao reconhecida.\n",iLineAtual,GetTokByIndex(1,szLineBuf," "),szSubRoutineContext);
			fprintf(stdout,"Conteudo da Linha [%s]\n",SupressNewLine(szLine));
			exit(1);
}
///////////////////////////////////////////////////////////////////////////////
char *ChangeParameters2LowCase(char *szstring)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,szstring);
			strcpy(szstring,"");
   buf=strtok(szLineBuf, " ");
			while(buf!=NULL)
			{
			   if(buf[0]==':')
						  strlower(buf);
			   if(buf[0]=='(' && buf[1]==':')
						  strlower(buf);
			   if(buf[0]=='=' && buf[1]==':')
						  strlower(buf);
		    strcat(szstring,buf);	
			   buf=strtok(NULL, " ");
						if(buf!=NULL)
						  strcat(szstring," ");	
			}
			return(szstring);
}

///////////////////////////////////////////////////////////////////////////////
int isExecSqlIncludeSqlcaStatement(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   buf=strstr(szLineBuf, "!");
   if(buf!=NULL)
     buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'
   
			buf=strtok(szLineBuf, " "); if(buf==NULL) return(0);
			
   if(strcmp(strlower(buf),"exec")==0)
			{
					buf=strtok(NULL, " "); if(buf==NULL) return(0);
     if(strcmp(strlower(buf),"sql")==0)
					{
							buf=strtok(NULL, " "); if(buf==NULL) return(0);
     		if(strcmp(strlower(buf),"include")==0)
							{
									buf=strtok(NULL, " "); if(buf==NULL) return(0);
     				if(strcmp(strlower(buf),"sqlca")==0)
									{
              return(1);
									}					
							}							
					}								
			}
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
void FindSubRoutineContextChange(char *szLine,FILE *fhProC,int iUseOutPut)
{
    char *buf;
    static char szLineBuf[(MAX_LINE)+1];

    if(strstr(szLine,"/* Main program */")==&szLine[0])
				{
				   if(GetTokByIndex(5,szLine," ")!=NULL)
							{
	        if(iUseOutPut)
									{						
         			fprintf(fhProC,"/* ********************************************************************** */ \n");
         			fprintf(fhProC,"/* Inicio programa principal                                              */ \n");									
         		//	fprintf(stdout,"/* ********************************************************************** */ \n");
         		//	fprintf(stdout,"/* Inicio programa principal                                              */ \n");									
         }
							  strcpy(szSubRoutineContext, GetTokByIndex(5,szLine," "));
									buf = strstr(szSubRoutineContext,"(");
									buf[0]=0x00;
							}		
				}					
    if(strstr(szLine,"/* Subroutine */")==&szLine[0])
				{
				   if(GetTokByIndex(4,szLine," ")!=NULL)
							{
	        if(iUseOutPut)
									{						
         			fprintf(fhProC,"/* ********************************************************************** */ \n");
         			fprintf(fhProC,"/* Inicio subrotina                                                       */ \n");									
         		//	fprintf(stdout,"/* ********************************************************************** */ \n");
         		//	fprintf(stdout,"/* Inicio subrotina                                                       */ \n");									
         }
							  strcpy(szSubRoutineContext, GetTokByIndex(4,szLine," "));
									buf = strstr(szSubRoutineContext,"(");
									buf[0]=0x00;
							}	
    }


    sprintf(szLineBuf,"} /* %s */",szSubRoutineContext);

    if(strstr(szLine,szLineBuf)==&szLine[0])
				{
       strcpy(szSubRoutineContext,"____GLOBALCONTEXT____");				
	      if(iUseOutPut)
							{						
       			fprintf(fhProC,"/* Fim                                                                    */ \n");									
       			fprintf(fhProC,"/* ********************************************************************** */ \n");
       		//	fprintf(stdout,"/* Fim                                                                    */ \n");									
       		//	fprintf(stdout,"/* ********************************************************************** */ \n");
							}		
				}		
				
}
///////////////////////////////////////////////////////////////////////////////
char *SolveCommentedLabels(char *szLine)
{
   int i;
   static char szLineBuf[(MAX_LINE)+1];
   if(szLine[0]=='/' && szLine[1]=='*' && szLine[2]==' ' && szLine[3]=='L' && isdigit(szLine[4]))
			{
			   i=4;
			   while(isdigit(szLine[i]))
						  i++; 
		    if(szLine[i]==':' && szLine[i+1]==' ' && szLine[i+2]=='*' && szLine[i+3]=='/')
						{
						   szLine[i+1]=';';
						   szLine[i+2]='\n';
						   szLine[i+2]=0x00;
						   strcpy(szLineBuf,strstr(szLine,"L"));
						   strcpy(szLine,szLineBuf);
									return(szLine);
						}	
			}
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
// /*Esta funcao verifica se a linha possui uma chamada do tipo abaixo. */
//  /*<       CALL EXIT >*/
//      exit_();
//  /*<       STOP >*/
//      s_stop("", (ftnlen)0);
//  /*<       END >*/
int isC_ExitOrStopCall(char* szLine)
{ 
   char *buf;
   buf=GetTokByIndex(0,szLine," ");
			if(buf==NULL)
			  return(0);		
			SupressNewLine(buf);
//   if( strstr(buf,"exit_();")!=NULL  || strstr(buf,"s_stop(" )!=NULL )
   if( strcmp(buf,"exit_();")==0 )
			  return(1);
			return(0);		
}
///////////////////////////////////////////////////////////////////////////////
void ProcessProCLine(char *szLine,FILE *fhProC)
{
   szLineBuf[0]=0x00;
			FindSubRoutineContextChange(szLine,fhProC,1);
   SolveCommentedLabels(szLine);

			if(strstr(szLine,"#include \"f2c.h\"")!=NULL)
			{
      fprintf(fhProC,szLine);
      fprintf(fhProC,"EXEC SQL INCLUDE SQLCA; \n");
      fprintf(fhProC,"#define sqlemc ConvC2FortranChar(sqlca.sqlerrm.sqlerrmc,70)\n\n");
						fprintf(fhProC,DEF_ConvFortranChar2C);
      fprintf(fhProC,"\n");
						fprintf(fhProC,DEF_ConvC2FortranChar);
      fprintf(fhProC,"\n");
      if(iSqlLogMode)
						{
						  fputs("void _proforSqlTraceOutput(char* szstring);\n",fhProC);
						  fputs("void _proforSqlTraceSQLCA(void);\n",fhProC);
						  fputs("void _proforSqlTraceOutputVarInteger(char* szstring,int dvar);\n",fhProC);
						  fputs("void _proforSqlTraceOutputVarDouble(char* szstring,double dvar);\n",fhProC);
						  fputs("void _proforSqlTraceOutputVarChar(char* szstring,char *dvar);\n",fhProC);
						}		
						return;
			}
						
   if(isPreprocessorComment(szLine) && isVariableDeclaration(szLine)) 
			{ 
			   SupressPreprocessorComment(szLine);
						ChangeParameters2LowCase(szLine);
			   fprintf(fhProC,AdjustVariableDeclaration(szLine)); 
      return; 

			}  

   // Modificacao para implementaçao do codigo C inline no FORTRAN
   if(isPreprocessorInlineComment(szLine))
			{
      SupressPreprocessorInlineComment(szLine);
      fputs(szLine,fhProC);			
						return; 
			}


   if(isPreprocessorComment(szLine)) 
			{ 
						ChangeParameters2LowCase(szLine);
						SupressPreprocessorComment(szLine);
						if(!isExecSqlIncludeSqlcaStatement(szLine))
						{
									fputs(szLine,fhProC);
						}
      return; 
			}  

   if(isC_ExitOrStopCall(szLine)) 
			{ 
						fputs("    exit(0);\n",fhProC); 
			   fputs(szLine,fhProC); 
			   return;
   } 
			
   
			fputs(szLine,fhProC);
}
///////////////////////////////////////////////////////////////////////////////
int isCommentedFortranContinuation(char* szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
			buf = GetTokByIndex(2,szLine," ");
			if(buf!=NULL)
			  if(strcmp(buf,"EXEC")==0)
					   return(0);
   if(isPreprocessorComment(szLine))
   {
   			strcpy(szLineBuf,szLine);
   			buf=strtok(szLineBuf, " "); 
						// Verifica se e´ um label
						if(buf!=NULL)            // Pula o "/*"       
   						buf=strtok(NULL, " ");
									if(buf!=NULL)         // Pula o "__PREPROC__"       
   									buf=strtok(NULL, " ");
												if(buf!=NULL)                     
 		   									if((atoi(buf)!=0) && (szLine[20]!=' '))
        									return(0);
							if(szLine[19]!=' ')										
     		    return(1);
				}				
   return(0);  
}
///////////////////////////////////////////////////////////////////////////////
char *SupressC_EndComment(char *szLine)
{
			int i;

			i = strlen(szLine)-1;
			while(i>=0)
			{
			   if(i-1==0) break;
						if(szLine[i-1]=='*' && szLine[i]=='/')
						{
						   szLine[i-1]=' ';
						   szLine[i]  =' ';
						}
						i--;
			}
			return(szLine);		
}
///////////////////////////////////////////////////////////////////////////////
void SupressExecSqlFortranLineContinuation(char *szFileIn,char *szFileOut)
{
  FILE *fhProFor,*fhFortran;
		int iUseLineAux;
  fprintf(stdout,"Retirando possiveis continuaçoes de linha com codigos \"exec sql\" no arquivo [%s]\n",szFileProFor);
  fprintf(stdout,"-----------------------------------------------------------------------\n");

  fhProFor  = OpenFile(szFileIn,"rb",1);
  fhFortran  = OpenFile(szFileOut,"wb",1);
  iUseLineAux=0;
		iLineAtual=0;
		strcpy(szAtualFile,szFileIn);
  fgets(szLine, MAX_LINE, fhProFor);
		MsgDebugLine(szLine);
  while(!feof(fhProFor))
  {
     if(isPreprocessorComment(szLine))
     {
        fgets(szLineAux, MAX_LINE, fhProFor);
		      MsgDebugLine(szLineAux);
								SupressNewLine(szLineAux);
								while(!feof(fhProFor))
								{
								    if(isCommentedFortranContinuation(szLineAux))
												{
											   SupressNewLine(szLine);
														SupressC_EndComment(szLine);
												  strcat(szLine," ");
												  strcat(szLine,AdjustTokens(&szLineAux[20]));
              fgets(szLineAux, MAX_LINE, fhProFor);
														MsgDebugLine(szLineAux);
	             SupressNewLine(szLineAux);
														strcat(szLine,"\n");
												}
												else
												{
					         // fprintf(fhFortran,szLine);
														// strcat(szLineAux,"\n");
												  // fprintf(fhFortran,szLineAux);
														// break;
														fputs(szLine,fhFortran);
														
														strcat(szLineAux,"\n");
														strcpy(szLine,szLineAux);
														iUseLineAux=1;
														break;
												} 
								}
     }
					else 
					{
					  iUseLineAux=0;
					  // fprintf(fhFortran,szLine);
							fputs(szLine,fhFortran);
					}		
					if(iUseLineAux==0)
					{
       fgets(szLine, MAX_LINE, fhProFor);
					  MsgDebugLine(szLine);
					}
  }
  fclose(fhProFor); 
  fclose(fhFortran); 
  fprintf(stdout,"-----------------------------------------------------------------------\n");
  fprintf(stdout,"Continuaçoes de linha retiradas\n",szFileFortran);
}
///////////////////////////////////////////////////////////////////////////////
void AppendInFortranVariableList(char *szNomeVar,char *szTypeVar,int iSize,char *szContext)
{
   if(iEndVariableList==DEF_MAX_VAR_WORK)
			{
			   fprintf(stdout,"[AppendInFortranVariableList] - Maximo de variaveis Fortran alcançado. Abortando o pre-compilador\n");
						exit(1);
			}
   strcpy(stListaVariables[iEndVariableList].name,szNomeVar);
   strcpy(stListaVariables[iEndVariableList].type,szTypeVar);
			stListaVariables[iEndVariableList].size = iSize;
   strcpy(stListaVariables[iEndVariableList].context,szContext);
	//		fprintf(stdout,"              Variavel FORTRAN -> %s - %s - %d - %s\n",szNomeVar,szTypeVar,iSize,szContext);
   iEndVariableList++;
}
///////////////////////////////////////////////////////////////////////////////
int IsFortranVarInThisContext(char *szNomeVar)
{
   int iSearch;
			char szBuffTemp[50];
			
			if(IsDeclareSqlGlobalFortranVar(szNomeVar))
			  return(1);
			
			strcpy(szBuffTemp,szNomeVar);
   iSearch=0;
			// printf("    [%s]\n",szBuffTemp);
   while(iSearch<iEndVariableList)
			{
			   if((strcmp(stListaVariables[iSearch].name,szNomeVar)==0) && (strcmp(stListaVariables[iSearch].context,szSubRoutineContext)==0))
						{
			     // printf("             [%s]\n",szBuffTemp);
			     // printf("             stListaVariables[%d].name -> [%s]\n",iSearch,stListaVariables[iSearch].name);
						  return(1);
						}		
      iSearch++;
			}
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
char* GetTypeFromFortranVar(char *szNomeVar)
{
   int iSearch;
   iSearch=0;
   while(iSearch<iEndVariableList)
			{
			   if((strcmp(stListaVariables[iSearch].name,szNomeVar)==0) && (strcmp(stListaVariables[iSearch].context,szSubRoutineContext)==0))
						  return(stListaVariables[iSearch].type);
      iSearch++;
			}
			return(NULL);
}
///////////////////////////////////////////////////////////////////////////////
int GetSizeFromFortranVar(char *szNomeVar)
{
   int iSearch;
   iSearch=0;
   while(iSearch<iEndVariableList)
			{
			   if((strcmp(stListaVariables[iSearch].name,szNomeVar)==0) && (strcmp(stListaVariables[iSearch].context,szSubRoutineContext)==0))
						  return(stListaVariables[iSearch].size);
      iSearch++;
			}
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
void SetFortranVarNameChangedStatus(char *szNomeVar)
{
   int iSearch;
   iSearch=0;
   while(iSearch<iEndVariableList)
			{
			   if((strcmp(stListaVariables[iSearch].name,szNomeVar)==0) && (strcmp(stListaVariables[iSearch].context,szSubRoutineContext)==0))
						  stListaVariables[iSearch].ischangedname=1;
      iSearch++;
			}
}
///////////////////////////////////////////////////////////////////////////////
int GetFortranVarNameChangedStatus(char *szNomeVar)
{
   int iSearch;
   iSearch=0;
   while(iSearch<iEndVariableList)
			{
			   if((strcmp(stListaVariables[iSearch].name,szNomeVar)==0) && (strcmp(stListaVariables[iSearch].context,szSubRoutineContext)==0))
						  return(stListaVariables[iSearch].ischangedname);
      iSearch++;
			}
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isStartLocalVariablesDeclaration(char *szLine)
{
   if(strstr(szLine,"/* Local variables */")!=NULL && szLine[4]=='/')
			  return(1);
			return(0);		
}
///////////////////////////////////////////////////////////////////////////////
int isEndLocalVariablesDeclaration(char *szLine)
{
   char *buf;
   buf=GetTokByIndex(0,szLine," ");
   if( (strstr(szLine,"/* Fortran I/O blocks */")!=NULL && szLine[4]=='/') || strcmp(buf,"\n")==0)
			  return(1);
			return(0);		
}
///////////////////////////////////////////////////////////////////////////////
char *SupressVarDeclaration(char *szLine,char *szVarName)
{
   static char szLineBuf[(MAX_LINE)+1];
			char *buf; int ind; char *buf1;
   int iHasComma;
			ind=0;
			strcpy(szLineBuf,szLine);
			strcpy(szLine,"   ");
			buf = GetTokByIndex(ind,szLineBuf," ");
			while(buf!=NULL)
			{
			   iHasComma = 0;
						buf1=strstr(buf,","); if(buf1!=NULL) { buf1[0]=0x00;	iHasComma = 1; } 		
						buf1=strstr(buf,"["); if(buf1!=NULL)   buf1[0]=0x00;
						buf1=strstr(buf,";"); if(buf1!=NULL)   buf1[0]=0x00;
						buf1=strstr(buf,"\n"); if(buf1!=NULL)  buf1[0]=0x00;
						
						if(strstr(buf,szVarName)==NULL)
						{
						    strcat(szLine," ");
										buf = GetTokByIndex(ind,szLineBuf," ");
						    buf1=strstr(buf,",");  if(buf1!=NULL) buf1[0]=0x00;	
						    buf1=strstr(buf,";");  if(buf1!=NULL) buf1[0]=0x00;
						    buf1=strstr(buf,"\n"); if(buf1!=NULL) buf1[0]=0x00;
						    strcat(szLine,buf);
										if(iHasComma)
										  strcat(szLine,",");
						}  
			   ind++;
			   buf = GetTokByIndex(ind,szLineBuf," ");
			}
			if(szLine[strlen(szLine)-1]==',') szLine[strlen(szLine)-1]=0x00;
			buf = GetTokByIndex(2,szLineBuf," ");
   if(buf==NULL)
			  return("\n");
					
			strcat(szLine,";\n");
   if(strcmp(buf,";")==0)			
			   szLine[0]=0x00;
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char *SupressFortranVariablesAndSolveAlteredNames(char *szLine)
{   
    static char szLineBuf[(MAX_LINE)+1];
    char szTempVarName[50];
				char *buf; int ind;
				int iChangedName;
				iChangedName=0;
    ind=1;
				strcpy(szLineBuf,szLine);
    while(GetTokByIndex(ind,szLineBuf," ")!=NULL)
				{
				   iChangedName=0;
				   strcpy(szTempVarName,GetTokByIndex(ind,szLineBuf," "));
							buf=strstr(szTempVarName,"["); if(buf!=NULL) buf[0]=0x00;
							buf=strstr(szTempVarName,";"); if(buf!=NULL) buf[0]=0x00;
							buf=strstr(szTempVarName,","); if(buf!=NULL) buf[0]=0x00;
							if(szTempVarName[strlen(szTempVarName)-1]=='_' && szTempVarName[strlen(szTempVarName)-2]=='_')
							{
							  iChangedName=1;
									szTempVarName[strlen(szTempVarName)-2]=0x00;
							}	
		     if(IsFortranVarInThisContext(szTempVarName)  && !IsDeclareSqlGlobalFortranVar(szTempVarName)) 
							{
         SupressVarDeclaration(szLine,szTempVarName);
									if(iChangedName)
            SetFortranVarNameChangedStatus(szTempVarName);
						 }
		     if(strcmp(szTempVarName,"sqlemc")==0)  // Retira todas as referencia a sqlemc
							{
         SupressVarDeclaration(szLine,szTempVarName);
						 }
				   ind++;
				}
    if (GetTokByIndex(2,szLine," ")==NULL)
				  strcpy(szLine,"\n");
				return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char *Insert__InVarDeclaration(char *szLine)
{
   char *buf;
			char szResto[50];

			buf=strstr(szLine,"[");
			if(buf!=NULL)
			{
			   strcpy(szResto,buf);
						buf[0]=0x00;
						strcat(szLine,"__");
						strcat(szLine,szResto);
						return(szLine);
			}

			buf=strstr(szLine,";");
			if(buf!=NULL)
			{
			   strcpy(szResto,buf);
						buf[0]=0x00;
						strcat(szLine,"__");
						strcat(szLine,szResto);
						return(szLine);
			}
   fprintf(stdout,"[Insert__InVarDeclaration] -> Linha %s com erro de sintaxe\n",szLine);
			exit(1);
}
///////////////////////////////////////////////////////////////////////////////
char *SolveAlteredNames(char *szLine)
{
    static char szLineBuf[(MAX_LINE)+1];
    char szTempVarName[50];
				char *buf;
				strcpy(szLineBuf,szLine);
				if(GetTokByIndex(2,szLineBuf," ")!=NULL)
      strcpy(szTempVarName,GetTokByIndex(2,szLineBuf," "));
    else
				  if(GetTokByIndex(1,szLineBuf," ")!=NULL)
        strcpy(szTempVarName,GetTokByIndex(1,szLineBuf," "));
				  else
				    return(szLine);
				buf = strstr(szTempVarName,"["); if(buf!=NULL) buf[0]=0x00;
				buf = strstr(szTempVarName,";"); if(buf!=NULL) buf[0]=0x00;
				if(GetFortranVarNameChangedStatus(szTempVarName))
        Insert__InVarDeclaration(szLine);
				return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char *ConvertionFortran2C_Code(char *szVar)
{
    static char szLineBuf[(MAX_LINE)+1];
				char *buf;
				buf = GetTypeFromFortranVar(szVar);
				szLineBuf[0]=0x00;
    if(buf!=NULL)
				{
				   if(strcmp(buf,"char")==0)
				     if(GetFortranVarNameChangedStatus(szVar))
           sprintf(szLineBuf,"    ConvFortranChar2C(%s__,%d);\n",szVar,GetSizeFromFortranVar(szVar)-1);
									else
           sprintf(szLineBuf,"    ConvFortranChar2C(%s,%d);\n",szVar,GetSizeFromFortranVar(szVar)-1);									  
				}
				return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *ConvertionC2Fortran_Code(char *szVar)
{
    static char szLineBuf[(MAX_LINE)+1];
				char *buf;
				buf = GetTypeFromFortranVar(szVar);
				szLineBuf[0]=0x00;
    if(buf!=NULL)
				{
				   if(strcmp(buf,"char")==0)
				     if(GetFortranVarNameChangedStatus(szVar))
           sprintf(szLineBuf,"    ConvC2FortranChar(%s__,%d);\n",szVar,GetSizeFromFortranVar(szVar)-1);
									else
           sprintf(szLineBuf,"    ConvC2FortranChar(%s,%d);\n",szVar,GetSizeFromFortranVar(szVar)-1);									  
				}
				return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateGlobal2SqlVarAttrib(char *szVar)
{
    static char szLineBuf[3*(MAX_LINE)+1];
    static char szLineBufTmp[(MAX_LINE)+1];
				char szVarBuf[50];
				char *buf;
    strcpy(szVarBuf,szVar);
				buf = strstr(szVarBuf,"_f2c"); if(buf!=NULL) buf[0]=0x00;
				
				buf = GetTypeFromFortranVar(szVar);
				szLineBuf[0]=0x00;
    if(buf!=NULL)
				{
				   GetSqlGlobalFortranVarStruct(szVarBuf);
				   if(strcmp(buf,"char")==0)
							{
				     if(GetFortranVarNameChangedStatus(szVar))
									{
           sprintf(szLineBuf,   "    memset(%s__,0x00,sizeof(char)*%d);\n",szVar,GetSizeFromFortranVar(szVar));
           sprintf(szLineBufTmp,"    strncpy(%s__,%s,%d);\n",szVar,szVarBuf,GetSizeFromFortranVar(szVar)-1);
											strcat(szLineBuf,szLineBufTmp);
									}		
									else
									{
           sprintf(szLineBuf,   "    memset(%s,0x00,sizeof(char)*%d);\n",szVar,GetSizeFromFortranVar(szVar));
           sprintf(szLineBufTmp,"    strncpy(%s,%s,%d);\n",szVar,szVarBuf,GetSizeFromFortranVar(szVar)-1);
								   strcat(szLineBuf,szLineBufTmp);
									}	
							}		
				   if(strcmp(buf,"int")==0 || strcmp(buf,"float")==0 || strcmp(buf,"double")==0)
							{
				     if(GetFortranVarNameChangedStatus(szVar))
           sprintf(szLineBuf,"    %s__ = %s;\n",szVar,szVarBuf);
									else
           sprintf(szLineBuf,"    %s = %s;\n",szVar,szVarBuf);
							}				
				}
				return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GenerateSqlVar2GlobalAttrib(char *szVar)
{
    static char szLineBuf[2*(MAX_LINE)+1];
	   static char szLineBufTmp[(MAX_LINE)+1];
				char szVarBuf[50];
				char *buf;
    strcpy(szVarBuf,szVar);
				buf = strstr(szVarBuf,"_f2c"); if(buf!=NULL) buf[0]=0x00;
				
				buf = GetTypeFromFortranVar(szVar);
				szLineBuf[0]=0x00;
    if(buf!=NULL)
				{
				   GetSqlGlobalFortranVarStruct(szVarBuf);
				   if(strcmp(buf,"char")==0)
							{
				     if(GetFortranVarNameChangedStatus(szVar))
           sprintf(szLineBuf,"    strncpy(%s__,%s,%d);\n",szVarBuf,szVar,GetSizeFromFortranVar(szVar)-1);
									else
           sprintf(szLineBuf,"    strncpy(%s,%s,%d);\n",szVarBuf,szVar,GetSizeFromFortranVar(szVar)-1);
							}				
				   if(strcmp(buf,"int")==0 || strcmp(buf,"float")==0 || strcmp(buf,"double")==0)
							{
				     if(GetFortranVarNameChangedStatus(szVar))
           sprintf(szLineBuf,"    %s__ = %s;\n",szVarBuf,szVar);
									else
           sprintf(szLineBuf,"    %s = %s;\n",szVarBuf,szVar);
						 }				
				}
				return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
void DebugVariable(char *szVar,FILE *fhProC)
{
			char *bufTmp;
			char *buf1;
			char bufVar[50];

			bufTmp = GetTypeFromFortranVar(szVar);
			if(bufTmp!=NULL)
			{
				 if(strcmp(bufTmp,"char")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarCharOutput(szVar),fhProC); 
				 if(strcmp(bufTmp,"float")==0 || strcmp(bufTmp,"double")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarDoubleOutput(szVar),fhProC); 
				 if(strcmp(bufTmp,"int")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarIntegerOutput(szVar),fhProC); 
						return;					
   }

			strcpy(bufVar,szVar);
			buf1 = strstr(bufVar,"__"); if(buf1!=NULL) { buf1[0]=0x00; }
			bufTmp = GetTypeFromFortranVar(bufVar);
			if(bufTmp!=NULL)
			{
				 if(strcmp(bufTmp,"char")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarCharOutput(szVar),fhProC); 
				 if(strcmp(bufTmp,"float")==0 || strcmp(bufTmp,"double")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarDoubleOutput(szVar),fhProC); 
				 if(strcmp(bufTmp,"int")==0)
								if(iSqlLogMode)
				      	fputs(GenerateSqlLogVarIntegerOutput(szVar),fhProC); 
   }

}
///////////////////////////////////////////////////////////////////////////////
void ExecSqlConvertionPosTasks(char *szLine,FILE *fhProC)
{
    static char szLineBuf[2*(MAX_LINE)+1];
    static char szLineBufTmp[(MAX_LINE)+1];
				char *buf; int i; 
				char *buf1;
				int iIsGlobalVar;
                                
				i=0; 
				strcpy(szLineBuf,szLine);
				buf=GetSqlTokenByIndex(i,szLineBuf);
				while(buf!=NULL)
				{
				   iIsGlobalVar=0; 
				   strcat(szLine," ");
				   buf1 = strstr(buf,"_f2c"); if(buf1!=NULL) { buf1[0]=0x00; }
				   buf1 = strstr(buf,"__"); if(buf1!=NULL) { buf1[0]=0x00; }

				   if(buf[0]==':' && IsFortranVarInThisContext(&buf[1]))
			    {
				      if(IsDeclareSqlGlobalFortranVar(&buf[1]) || GetFortranVarNameChangedStatus(&buf[1])) 
				      { 
					        iIsGlobalVar=1;
													if(IsDeclareSqlGlobalFortranVar(&buf[1]))
				           strcat(&buf[1],"_f2c"); 
													if(GetFortranVarNameChangedStatus(&buf[1]))
				           strcat(&buf[1],"__"); 
             fprintf(fhProC,GenerateSqlVar2GlobalAttrib(&buf[1]));
				      } 
										// fprintf(fhProC,ConvertionC2Fortran_Code(&buf[1])); 
          DebugVariable(&buf[1],fhProC);
				   }
       i++;
       buf=GetSqlTokenByIndex(i,szLineBuf);
     }
}
///////////////////////////////////////////////////////////////////////////////
void ExecSqlConvertionPreTasks(char *szLine,FILE *fhProC)
{
    static char szLineBuf[(MAX_LINE)+1];
    char *buf; int i; int iHasComma; int iHasParenthesis; int iHasParComma;
    char *buf1;
    char *bufTmp;
    int iIsGlobalVar;

    strcpy(szLineBuf,szLine);
    strcpy(szLine,"");
    i=0; iHasComma=0; iIsGlobalVar=0; iHasParenthesis=0; iHasParComma=0;
    buf=GetSqlTokenByIndex(i,szLineBuf);
    
    while(buf!=NULL)
    {
      iHasComma=0; iIsGlobalVar=0; iHasParenthesis=0;
										
      buf1 = strstr(buf,"="); 
      if( buf1!=NULL && strcmp(buf,"=")!=0 )
				  {
									fprintf(stdout,"\n\nATENCAO: - %s - Sintaxe nao aceita pelo prof. Comparaçao de variaveis sem espaco entre elas. \n\n",szFileC);
									fprintf(stdout,"Verificar todas as clausulas EXEC SQL onde ocorre \"WHERE VAR1=:var2\" e corrigir substituindo por \"WHERE VAR1 = :var2\"\n\n");
			      fprintf(stdout,"Conteudo da Linha [%s]\n\n",SupressNewLine(szLineBuf));
									exit(1);
      } 

      if(buf[0]==':' && IsFortranVarInThisContext(&buf[1]))
						{

						   if(IsDeclareSqlGlobalFortranVar(&buf[1])) 
									{ 
										  iIsGlobalVar=1;
				        strcat(&buf[1],"_f2c"); 
            fprintf(fhProC,GenerateGlobal2SqlVarAttrib(&buf[1]));
									} 

         fprintf(fhProC,ConvertionFortran2C_Code(&buf[1]));

         if(GetFortranVarNameChangedStatus(&buf[1]))
										  strcat(buf,"__");
         DebugVariable(&buf[1],fhProC);
						}

						strcat(szLine,buf);
						i++;
						buf=GetSqlTokenByIndex(i,szLineBuf);
   }
}
///////////////////////////////////////////////////////////////////////////////
void ResolveExecSqlConvertionsAndGlobalsFromFortran2C(char *szLine,FILE *fhProC)
{
    static char szLocBuf[(MAX_LINE)+1];
				if(iSqlLogMode && strstr(szLine,"EXEC SQL INCLUDE SQLCA;")==NULL)
				{
				   fputs("_proforSqlTraceOutput(\"-------------------------------------------------------------------------------\");",fhProC); 
				   fputs("\n",fhProC);
				}			
    ExecSqlConvertionPreTasks(szLine,fhProC);
				if(iSqlLogMode)
				{
				   SupressNewLine(szLine);
				   fputs(GenerateSqlTraceSourceLine(szLine),fhProC); 
							strcat(szLine,"\n");
				}			
				fputs(szLine,fhProC); /* Original exec sql line */
    ExecSqlConvertionPosTasks(szLine,fhProC);
				if(iSqlLogMode)
				{
				   SupressNewLine(szLine);
				   fputs(GenerateSqlTraceSQLCA(szLine),fhProC); 
							strcat(szLine,"\n");
				}			
				
}
///////////////////////////////////////////////////////////////////////////////
char *SupressIntraLineC_Comments(char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
   int i,iend,iComment,iBuf;
			iComment = 0;
			i = 0;
			iBuf = 0;
   iend = strlen(szLine);
			while(i<iend)
			{
			   if(szLine[i]=='/' && szLine[i+1]=='*')
						{
						  iComment = 1;
								i+=2;
						}		
			   if(szLine[i]=='*' && szLine[i+1]=='/')
						{
						  iComment = 0;
								i+=2;
						}		
						if(!iComment)
						{
						  szLineBuf[iBuf]=szLine[i];
								iBuf++;
						}		
			   i++;
			}
			szLineBuf[iBuf]=0x00;
			strcpy(szLine,szLineBuf);
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char *SupressSpacesInCharPointerDeclarationsSize(char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
   int i,iend,iSizeDec,iBuf;
			iSizeDec = 0;
			i = 0;
			iBuf = 0;
   iend = strlen(szLine);
			while(i<iend)
			{
			   if(szLine[i]=='[')
						  iSizeDec = 1;

			   if(szLine[i]==']')
						  iSizeDec = 0;

						if(!iSizeDec)
						{
						  szLineBuf[iBuf]=szLine[i];
								iBuf++;
						}		
						else if(szLine[i]!=' ')
						{
						   szLineBuf[iBuf]=szLine[i];
									iBuf++;
						}
			   i++;
			}
			szLineBuf[iBuf]=0x00;
			strcpy(szLine,szLineBuf);
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char *SolveFakeEndOfLineAndSupressComments(char *szLine,FILE *fhInput)
{
   static char szLineBuffer[(MAX_LINE)+1]; 
			SupressNonValidCharacters(szLine);
   if(strstr(szLine,";")==NULL)
			{
			   SupressNewLine(szLine);
			   fgets(szLineBuffer, MAX_LINE, fhInput);
						MsgDebugLine(szLineBuffer);
						SupressNonValidCharacters(szLineBuffer);
						while(strstr(szLineBuffer,";")==NULL)
						{
										strcat(szLine,szLineBuffer);  						   
										SupressNewLine(szLine);
										fgets(szLineBuffer, MAX_LINE, fhInput);
										MsgDebugLine(szLineBuffer);
										SupressNonValidCharacters(szLineBuffer);
						}
						strcat(szLine,szLineBuffer);  						   
			}
			SupressIntraLineC_Comments(szLine);
			SupressSpacesInCharPointerDeclarationsSize(szLine);
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
void SolveVariableDeclarationsAndTypes(char *szLine,FILE *fhProC,FILE *fhTmp)
{
   static int isLocalVariablesSection;
   static int isSqlDeclareSection;
   szLineBuf[0]=0x00;
			
			FindSubRoutineContextChange(szLine,fhProC,0);
   
			if(isStartLocalVariablesDeclaration(szLine)) 
			{
     isLocalVariablesSection=1;
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}		
   if(isEndLocalVariablesDeclaration(szLine)) 
			{
     isLocalVariablesSection=0;
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}		

   if(isExecSqlBeginDeclareSectionStatement(szLine))
			{
			  isSqlDeclareSection=1;
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}
			
   if(isExecSqlEndDeclareSectionStatement(szLine))
			{
			  isSqlDeclareSection=0;
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}
 
	  if(isSqlDeclareSection)
			{
			  SolveAlteredNames(szLine);
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}
	
	  if(isLocalVariablesSection)
			{
			  SolveFakeEndOfLineAndSupressComments(szLine,fhTmp);
			  SupressFortranVariablesAndSolveAlteredNames(szLine);
     // fprintf(fhProC,szLine);
					fputs(szLine,fhProC);
					return;
			}
   
		if(isExecSqlStatement(szLine))
		{
     ResolveExecSqlConvertionsAndGlobalsFromFortran2C(szLine,fhProC);
					// fputs(szLine,fhProC);
		   return;
		}
			
   // fprintf(fhProC,szLine);
			fputs(szLine,fhProC);
}
///////////////////////////////////////////////////////////////////////////////
void SolveVariableConvertionsAndDeclarations(void)
{
  char szCommand[512];
		FILE* fhTmp;
		FILE* fhProC;
  char szTmpFile[512];
		
  fprintf(stdout,"-----------------------------------------------------------------------\n");
		fprintf(stdout,"Resolvendo declaraçoes e conversao de tipos C/FORTRAN do arquivo PRO*C [%s]\n",szFileProC);
  fprintf(stdout,"-----------------------------------------------------------------------\n");
  sprintf(szCommand,"cp %s %s.tmp",szFileProC,szFileProC);
		system(szCommand);
  strcpy(szTmpFile,szFileProC);
		strcat(szTmpFile,".tmp");
		
  fhTmp  = OpenFile(szTmpFile,"rb",1);
  fhProC  = OpenFile(szFileProC,"wb",1);
  strcpy(szSubRoutineContext,"____GLOBALCONTEXT____");				
		iLineAtual=0;
		strcpy(szAtualFile,szTmpFile);
  fgets(szLine, MAX_LINE, fhTmp);
		MsgDebugLine(szLine);
  while(!feof(fhTmp))
  {
     SolveVariableDeclarationsAndTypes(szLine,fhProC,fhTmp);
     fgets(szLine, MAX_LINE, fhTmp);
					MsgDebugLine(szLine);
  }
  fclose(fhTmp);
  fclose(fhProC);
		
		sprintf(szCommand,"rm -rf %s.tmp",szFileProC);
		system(szCommand);

  fprintf(stdout,"-----------------------------------------------------------------------\n");
}
///////////////////////////////////////////////////////////////////////////////
void UpdateCommonStructName(char *szVarName,char *szStructName)
{
   int iSearch;
			iSearch=0;
   while(iSearch<iEndCommonVariableList)
			{
			   if(strcmp(stListaCommonVariables[iSearch].name,szVarName)==0)
						{
         strcpy(stListaCommonVariables[iSearch].structname,SupressNewLine(szStructName));
								 return;
						}		
      iSearch++;
			}
			return;
}
///////////////////////////////////////////////////////////////////////////////
char* SupressEndOfLine__(char* szLine)
{
   int iLen;
			iLen=strlen(szLine);
   if(szLine[iLen-1]=='_' && szLine[iLen-2]=='_') 
			{
			  szLine[iLen-1]=0x00;
			  szLine[iLen-2]=0x00;
   }					
			return(szLine);		
}
///////////////////////////////////////////////////////////////////////////////
void LoadRealVariableNamesInCommonStruct(char *szLine)
{
   char szVarName[50];
   char szVarNameNo__[50];
   if(strstr(szLine,"#define") == &szLine[0])
			{
			   if(GetTokByIndex(1,szLine," ")!=NULL)
						{
						   strcpy(szVarName,GetTokByIndex(1,szLine," "));
						   strcpy(szVarNameNo__,GetTokByIndex(1,szLine," ")); 
									SupressEndOfLine__(szVarNameNo__);
								 if(IsCommonFortranVar(szVarName))
									  UpdateCommonStructName(szVarName,GetTokByIndex(2,szLine," "));
								 if(IsCommonFortranVar(szVarNameNo__))
									  UpdateCommonStructName(szVarNameNo__,GetTokByIndex(2,szLine," "));
						}		
			} 
}
///////////////////////////////////////////////////////////////////////////////
void LoadStructNamesToCommonTable(char *szFileName)
{
		FILE* fhTmp;
  char szCommand[512];
		sprintf(szCommand,"cp %s tmp_data_struct.f",szFileFortran);
		system(szCommand);
		// sprintf(szCommand,"f2c -p -Nn802 tmp_data_struct.f > /dev/null 2>&1");
		sprintf(szCommand,"f2c -c -p -Nn802 tmp_data_struct.f > /dev/null 2>&1");
		system(szCommand);
		
		fprintf(stdout,"Carregando Estrutura das Variaveis COMMON\n");
		
  fhTmp  = OpenFile("tmp_data_struct.c","rb",1);
		iLineAtual=0;
  fgets(szLine, MAX_LINE, fhTmp);
		MsgDebugLine(szLine);
  while(!feof(fhTmp))
  {
     LoadRealVariableNamesInCommonStruct(szLine);
     fgets(szLine, MAX_LINE, fhTmp);
					MsgDebugLine(szLine);
  }
  fclose(fhTmp);
		
		sprintf(szCommand,"rm -rf tmp_data_struct.f");
		system(szCommand);
		sprintf(szCommand,"rm -rf tmp_data_struct.c");
		system(szCommand);

}
///////////////////////////////////////////////////////////////////////////////
void ConvertProFortranFile2ProC(void)
{
  char szCommand[512];
		FILE* fhC;
		FILE* fhProC;

		strcpy(szFileC,IgnoreExtension(szFileFortran));
		strcpy(szFileProC,IgnoreExtension(szFileFortran));
		strcat(szFileC,".c");
		strcat(szFileProC,".pc");


		fprintf(stdout,"Convertendo arquivo [%s] de FORTRAN para C\n",szFileFortran);
  fprintf(stdout,"-----------------------------------------------------------------------\n");
		sprintf(szCommand,"f2c -c -Nn802 %s 2>&1",szFileFortran);
  if(system(szCommand)!=0)
		{
  			fprintf(stdout,"Erro de na conversao de Fortran para C\n");
  			fprintf(stdout,"verificar as mensagens de erro do f2c.\n");
					exit(1);
		}
		LoadStructNamesToCommonTable(szFileFortran);
  fprintf(stdout,"-----------------------------------------------------------------------\n");
		fprintf(stdout,"Gerando arquivo PRO*C [%s] a partir do arquivo C [%s]\n",szFileProC,szFileC);
  fprintf(stdout,"-----------------------------------------------------------------------\n");

		SupressExecSqlFortranLineContinuation(szFileC,"auxtmp.data");
  sprintf(szCommand,"cp auxtmp.data %s",szFileC);
		system(szCommand);

  sprintf(szCommand,"rm -rf auxtmp.data");
		system(szCommand);

  fhC  = OpenFile(szFileC,"rb",1);
  fhProC  = OpenFile(szFileProC,"wb",1);
  iLineAtual=0;  
		strcpy(szAtualFile,szFileC);
  fgets(szLine, MAX_LINE, fhC);
		MsgDebugLine(szLine);
  while(!feof(fhC))
  {
     ProcessProCLine(szLine,fhProC);
     fgets(szLine, MAX_LINE, fhC);
		   MsgDebugLine(szLine);
  }
  if(iSqlLogMode)
		{
				fprintf(fhProC,"\n#include <stdio.h>\n");

				fprintf(fhProC,"#define DEF_SQLLOGPROFOROUTPUT \"%s\"\n",SupressNewLine(sz_proforSqlTraceOutput));
				fprintf(fhProC,"#define DEF_PROFORSOURCE \"%s\"\n",SupressNewLine(szFileProFor));
				fputs(DEF_proforIsNewSqlTraceOutput,fhProC);
				fputs(DEF_proforSqlTraceOutput,fhProC);
				fputs(DEF_proforSqlTraceSQLCA,fhProC);
				fputs(DEF_proforSqlTraceOutputVarDouble,fhProC);
				fputs(DEF_proforSqlTraceOutputVarInteger,fhProC);
				fputs(DEF_proforSqlTraceOutputVarChar,fhProC);
		}		
  fclose(fhC);
  fclose(fhProC);
  fprintf(stdout,"-----------------------------------------------------------------------\n");
		SolveVariableConvertionsAndDeclarations();
}
///////////////////////////////////////////////////////////////////////////////
// struct {
//   char name[50];
// } stListaChangePositionFlag[DEF_MAX_VAR_WORK];
// int iEndListaChangePositionFlag;
///////////////////////////////////////////////////////////////////////////////
void PutContextInChangePositionTable(void)
{
   strcpy(stListaChangePositionFlag[iEndListaChangePositionFlag].name,szSubRoutineContext);
   iEndListaChangePositionFlag++;
}
///////////////////////////////////////////////////////////////////////////////
int iHasFormatIODeclarationInThisContext(void)
{
   int i=0;
			while(i<iEndListaChangePositionFlag) 
			{
       if(strcmp(stListaChangePositionFlag[i].name,szSubRoutineContext)==0)
         return(1);
							i++;		
			}
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isConvertedFortranFormat(char *szLine)
{
   char *buf;
   static int iLineCont;
			
   if(strstr(szLine,"/* Fortran I/O blocks */")!=NULL && szLine[4]=='/')
			{
			   iLineCont=0;
			   return(1);
			}		
					
   buf=GetTokByIndex(0,szLine," ");
   if(strstr(szLine,"static cilist io___")!=NULL && strcmp(GetTokByIndex(0,szLine," "),"static")==0 && strcmp(GetTokByIndex(1,szLine," "),"cilist")==0)
			{
			   iLineCont=1;
			   return(1);
			}		
   if(strstr(szLine,"static icilist io___")!=NULL && strcmp(GetTokByIndex(0,szLine," "),"static")==0 && strcmp(GetTokByIndex(1,szLine," "),"icilist")==0)
			{
			   iLineCont=1;
			   return(1);
			}		
   if(!isOnlyNewLine(szLine) && iLineCont==1)
			{
			   return(1);
			}		
			
			iLineCont=0;
			return(0);		
}
///////////////////////////////////////////////////////////////////////////////
void PutLineInBuffer(char *szLine)
{
 		FILE* fhTemp;
	 	fhTemp = fopen("bufiolist.dat","a+b");
			if(fhTemp==NULL)
			  return;
	 	fputs(szLine,fhTemp);
   fclose(fhTemp);
}
///////////////////////////////////////////////////////////////////////////////
void OutputAndFlushBuffer(FILE *fhProC)
{
   char szLineAux[(MAX_LINE)+1];
 		FILE* fhTemp;
   char szCommand[512];
			
	 	fhTemp = fopen("bufiolist.dat","r");
			if(fhTemp==NULL)
			  return;
   fgets(szLineAux, MAX_LINE, fhTemp);
   while(!feof(fhTemp))
   {
	 	   fputs(szLineAux,fhProC);
      fgets(szLineAux, MAX_LINE, fhTemp);
   }
   fclose(fhTemp);

   sprintf(szCommand,"rm -rf bufiolist.dat");
 		system(szCommand);
}
///////////////////////////////////////////////////////////////////////////////
void ProcessChangePositionLine(char *szLine,FILE* fhProC)
{
   szLineBuf[0]=0x00;
			
			FindSubRoutineContextChange(szLine,fhProC,0);
				
   if(isConvertedFortranFormat(szLine) && iHasFormatIODeclarationInThisContext()) 
			{ 
			   PutLineInBuffer(szLine);
      return; 
			}  

   if(isExecSqlEndDeclareSectionStatement(szLine) && iHasFormatIODeclarationInThisContext()) 
			{ 
						fputs(szLine,fhProC);
						OutputAndFlushBuffer(fhProC);
      return; 
			}  
			fputs(szLine,fhProC);
}
///////////////////////////////////////////////////////////////////////////////
void MountChangePositionTable(char *szLine,FILE* fhProC)
{
			static int iFlag;
			FindSubRoutineContextChange(szLine,fhProC,0);
						
   if(isConvertedFortranFormat(szLine)) 
			{ 
			   iFlag=1;
      return; 
			}  

   if(isExecSqlEndDeclareSectionStatement(szLine) && iFlag) 
			{ 
			   PutContextInChangePositionTable();
      return; 
			}  
}
///////////////////////////////////////////////////////////////////////////////
void	ChangeExecSqlDeclarePosition2BeforeFormat(void)
{
  char szCommand[512];
		FILE* fhC;
		FILE* fhProC;

		fprintf(stdout,"Ajustando a Posiçao das Declaracoes FORTRAN e IO FORMAT no Arquivo [%s]\n",szFileProC);
  sprintf(szCommand,"cp %s auxtmp.data",szFileProC);
		system(szCommand);

		strcpy(szSubRoutineContext,"____GLOBALCONTEXT____");

  fhC  = OpenFile("auxtmp.data","rb",1);
  fhProC  = OpenFile(szFileProC,"wb",1);
  iLineAtual=0;  
		strcpy(szAtualFile,szFileProC);
  fgets(szLine, MAX_LINE, fhC);
  while(!feof(fhC))
  {
     MountChangePositionTable(szLine,fhProC);
     fgets(szLine, MAX_LINE, fhC);
  }
  fclose(fhC);
  fclose(fhProC);

		strcpy(szSubRoutineContext,"____GLOBALCONTEXT____");

  fhC  = OpenFile("auxtmp.data","rb",1);
  fhProC  = OpenFile(szFileProC,"wb",1);
  iLineAtual=0;  
		strcpy(szAtualFile,szFileC);
  fgets(szLine, MAX_LINE, fhC);
		MsgDebugLine(szLine);
  while(!feof(fhC))
  {
     ProcessChangePositionLine(szLine,fhProC);
     fgets(szLine, MAX_LINE, fhC);
		   MsgDebugLine(szLine);
  }
  fclose(fhC);
  fclose(fhProC);

  sprintf(szCommand,"rm -rf auxtmp.data");
		system(szCommand);
}
///////////////////////////////////////////////////////////////////////////////
int isFortranComment(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   buf=strtok(szLineBuf, " ");
			if(buf!=NULL)                     // Testa se na verdade a linha e´ um label 
 		   if((atoi(buf)!=0) && (line[1]!=' '))
        return(0);

   if(line[0]=='c' || line[0]=='c'|| line[0]=='*')
     return(1);
   if(line[0]!=' ')
     return(1);
   return(0);  
}
///////////////////////////////////////////////////////////////////////////////
void AddCommaToEndLine(char* szLine)
{
   if(szLine[strlen(szLine)-1]=='\n') 
			{
			  szLine[strlen(szLine)-1]=0x00;
					strcat(szLine,";\n"); 
			}		
}
///////////////////////////////////////////////////////////////////////////////
char* SupressNewLine(char* szLine)
{
   if(szLine[strlen(szLine)-1]=='\n') 
			  szLine[strlen(szLine)-1]=0x00;
			return(szLine);		
}
///////////////////////////////////////////////////////////////////////////////
int isExecSqlBeginDeclareSectionStatement(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   // buf=strstr(szLineBuf, "!");
   // if(buf!=NULL)
    // buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'
   
			buf=strtok(szLineBuf, " "); if(buf==NULL) return(0);
			
   if(strcmp(strlower(buf),"exec")==0)
			{
					buf=strtok(NULL, " "); if(buf==NULL) return(0);
     if(strcmp(strlower(buf),"sql")==0)
					{
							buf=strtok(NULL, " "); if(buf==NULL) return(0);
     		if(strcmp(strlower(buf),"begin")==0)
							{
									buf=strtok(NULL, " "); if(buf==NULL) return(0);
     				if(strcmp(strlower(buf),"declare")==0)
									{
											buf=strtok(NULL, " "); if(buf==NULL) return(0);
     				  if(strcmp(SupressNewLine(strlower(buf)),"section")==0)
              return(1);
									}					
							}							
					}								
			}
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isExecSqlEndDeclareSectionStatement(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   // buf=strstr(szLineBuf, "!");
   // if(buf!=NULL)
   //  buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'
   
			buf=strtok(szLineBuf, " "); if(buf==NULL) return(0);
   if(strcmp(strlower(buf),"exec")==0)
			{
					buf=strtok(NULL, " "); if(buf==NULL) return(0);
     if(strcmp(strlower(buf),"sql")==0)
					{
							buf=strtok(NULL, " "); if(buf==NULL) return(0);
     		if(strcmp(strlower(buf),"end")==0)
							{
									buf=strtok(NULL, " "); if(buf==NULL) return(0);
     				if(strcmp(strlower(buf),"declare")==0)
									{
											buf=strtok(NULL, " "); if(buf==NULL) return(0);
     				  if(strcmp(SupressNewLine(strlower(buf)),"section")==0)
              return(1);
									}					
							}							
					}								
			}
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isExecSqlStatement(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   strcpy(szLineBuf,line);
   // buf=strstr(szLineBuf, "!");
   // if(buf!=NULL)
   //   buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'
     
			buf = strtok(szLineBuf, " "); if(buf==NULL) return(0);
   if(strcmp(strlower(buf),"exec")==0)
			{
	    buf = strtok(NULL, " "); if(buf==NULL) return(0);
     if(strcmp(strlower(buf),"sql")==0)
       return(1);
			}				
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isExecSqlStatementAndLabel(char *line)
{
   static char szLineBuf[(MAX_LINE)+1];
   char *buf;
   int inum;
   strcpy(szLineBuf,line);
   buf=strstr(szLineBuf, "!");
   if(buf!=NULL)
     buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'

   buf = strtok(szLineBuf, " ");
   if(buf!=NULL)
   {
						if(atoi(buf)!=0)
						{
									buf = strtok(NULL, " "); if(buf==NULL) return(0);
									if(strcmp(strlower(buf),"exec")==0)
									{
												buf = strtok(NULL, " "); if(buf==NULL) return(0);
												if(strcmp(strlower(buf),"sql")==0)
			 											return(1);
							}	 
					}
   }
   return(0);
}
///////////////////////////////////////////////////////////////////////////////
int isFortranContinuation(char *line)
{
   if(GetTokByIndex(0,line," ")!=NULL)
			    if(strcmp(GetTokByIndex(0,line," "),"\n")==0)
							   return(0);
   if(line[5]!=' ' && line[5]!='\t' && line[0]!='\n')
     return(1);   
			return(0);			   
}
///////////////////////////////////////////////////////////////////////////////
void CorrectSqlFortranDoCall(char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
			char *szToken;
			int iNextIsRoutine;
   char *buf;
   strcpy(szLineBuf,szLine);

			if(strstr(szLineBuf,"do")==NULL && strstr(szLineBuf,"DO")==NULL)
			  return;
			if(strstr(szLineBuf,"call")==NULL && strstr(szLineBuf,"CALL")==NULL)
			  return;
			
   buf=strstr(szLineBuf, "!");
   if(buf!=NULL)
     buf[0]=0x00; // Ignora o conteudo da linha apos o caracter '!'

   strcpy(szLine,"      ");
			szToken=strtok(szLineBuf, " ");
			strcat(szLine,szToken);
			strcat(szLine," ");
			
			szToken=strtok(NULL, " ");
			iNextIsRoutine=0;
   while(szToken!=NULL)		
			{
	     if(strcmp(strlower(szToken),"call")!=0)		   
						{
								strcat(szLine,szToken);
								if(iNextIsRoutine)
								{
								  // strlower(szToken);
										// strcat(szLine,szToken);
								  if(szLine[strlen(szLine)-1]=='\n') szLine[strlen(szLine)-1]=0x00;
			       strcat(szLine,"_()");
										iNextIsRoutine=0;
								}		
								else
			       strcat(szLine," ");
						}
						else
						  iNextIsRoutine=1;
			   szToken=strtok(NULL, " ");
			}
			strcat(szLine,"\n");
   return;
}
///////////////////////////////////////////////////////////////////////////////
char *IgnoreExtension(char *szFileName)
{
   static char szTemp[512];
   int i = 0;
   while(szFileName[i]!='.' && szFileName[i]!=0x00) 
   {
     szTemp[i] = szFileName[i];
     i++; 
   }
   szTemp[i]=0x00;
   return szTemp;
}
///////////////////////////////////////////////////////////////////////////////
char *PutDeclareSectionId(char *szLine,int id)
{
   if(isExecSqlBeginDeclareSectionStatement(szLine) || isExecSqlEndDeclareSectionStatement(szLine))
			{
     sprintf(szLine,"%s ID_%05d\n",SupressNewLine(szLine),id);
					return szLine;
			}		
   return szLine;
}
///////////////////////////////////////////////////////////////////////////////
char *VariablePutDeclareSectionId(char *szLine,int id)
{
   char *buf;
   static char szLineBuf[(MAX_LINE)+1];
			char szBuf1[50];
			char szBuf2[50];
			
			strcpy(szLineBuf,szLine);
			strlower(szLineBuf);
   buf = GetTokByIndex(0,szLineBuf," ");
			if(buf==NULL)
			  return(szLine);
			if(isFortranVariableDeclaration(buf))
			{
			    strcpy(szBuf1,GetTokByIndex(0,szLineBuf," "));
			    strcpy(szBuf2,GetTokByIndex(1,szLineBuf," "));
       sprintf(szLine,"      %s %sID_%05d\n",szBuf1,SupressNewLine(szBuf2),id);
			}				
   return szLine;
}
///////////////////////////////////////////////////////////////////////////////
char *ChangeLine2PreprocessComment(char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
   sprintf(szLineBuf,"C__PREPROC__%s",szLine);
   strcpy(szLine,szLineBuf);
			return szLine;
}
///////////////////////////////////////////////////////////////////////////////
int isOnlyNewLine(char *szLine)
{
    if(strcmp(GetTokByIndex(0,szLine," "),"\n")==0)
				   return(1);
				return(0);		
}
///////////////////////////////////////////////////////////////////////////////
char *GetDeclarationItemTokenByIndex(int index,char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
			char szBuf[2];
			int i,indtok,iStartedNewToken;
			szLineBuf[0]=0x00;
			i = 0;
			indtok =-1;
			iStartedNewToken=0;
			while(indtok<index)
			{
			    if(szLine[i]==0x00)
							  return(NULL); 
						 if(iStartedNewToken == 0 && szLine[i]!=' ' && szLine[i]!=',' && szLine[i]!='\n' && szLine[i]!='=')
							{
          iStartedNewToken = 1;
										szLineBuf[0]=0x00;
							}			
						 if(iStartedNewToken == 1 && (szLine[i]==' ' || szLine[i]==',' || szLine[i]=='\n' || szLine[i]=='=') )
							{
          iStartedNewToken = 0;
										indtok++;
							}			
			    if(iStartedNewToken)
							{
							   szBuf[0]=szLine[i];  szBuf[1]=0x00;
										strcat(szLineBuf,szBuf);
							}
							i++;
			}
   return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
char *GetSqlTokenByIndex(int index,char *szLine)
{
   static char szLineBuf[(MAX_LINE)+1];
			char szBuf[2];
			int i,indtok;
			
			szLineBuf[0]=0x00;
			i = 0;
			indtok =0;
			while(indtok<=index)
			{
			    if(szLine[i]==0x00)
							  return(NULL); 
							
			    if( szLine[i]==' ' || szLine[i]=='\'' || szLine[i]==';' ||szLine[i]=='\n' || szLine[i]=='=' || szLine[i]==',' || szLine[i]=='(' || szLine[i]==')')
							{
							   if(indtok==index)
										  return(szLineBuf);
          indtok++;
							   if(indtok==index)
										{
             szBuf[0]=szLine[i];  szBuf[1]=0x00;
										   strcpy(szLineBuf,szBuf);
										   return(szLineBuf);
										}		
			       szLineBuf[0]=0x00;
										indtok++;
          i++;
										continue;
							}
							szBuf[0]=szLine[i];  szBuf[1]=0x00;
							strcat(szLineBuf,szBuf);
							i++;
			}
   return(szLineBuf);
}
///////////////////////////////////////////////////////////////////////////////
void AdjustFortranMultiVariableDeclaration(char *szLine,FILE *fhFortran)
{
    char szType[50];
				static char szLineBuf[(MAX_LINE)+1];
				char *buf;
				int i;
				strcpy(szType,GetDeclarationItemTokenByIndex(0,szLine));
				i=1;
				buf=GetDeclarationItemTokenByIndex(i,szLine);
				while(buf!=NULL)
				{
				   // Esta modificaçao faz o preprocessador sempre tratar as varivaveis como vari´avel global
							// para retirada de bugs inesperados com os causados pela formataçao de IO de vari´aveis
							// do banco. 
				   if(!IsCommonFortranVar(buf))
				      sprintf(szLineBuf,"       %s %s\n",szType,buf);
							else
				      sprintf(szLineBuf,"       %s %s_f2c\n",szType,buf);
							ChangeLine2PreprocessComment(szLineBuf);
							fputs(szLineBuf,fhFortran);
				   i++;
							buf=GetDeclarationItemTokenByIndex(i,szLine);
				}
}
///////////////////////////////////////////////////////////////////////////////
char *CorrectStartColumnOfExecSql(char *szLine)
{
			static char szLineBuf[(MAX_LINE)+1];
   char *buf;
			buf=strstr(szLine,"EXEC");
			strcpy(szLineBuf,"       ");
			strcat(szLineBuf,buf);
			strcpy(szLine,szLineBuf);
   return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
char* CorrectSqlFortranGoto(char *szLine)
{
			static char szLineBuf[(MAX_LINE)+1];
			char szToken[50];
   int i,iNextIsLabel;
			char *buf;
			if(strstr(szLine,"GOTO")==NULL && strstr(szLine,"goto")==NULL)
			  return(szLine);
			szLineBuf[0]=0x00;
   i=0; iNextIsLabel=0;
			buf=GetTokByIndex(i,szLine," ");
			strcpy(szLineBuf,"      ");
			while(buf!=NULL)
			{
			   strcat(szLineBuf," ");
						SupressNewLine(buf);
						if(iNextIsLabel)
						{
						   strcpy(szToken,"L");  
						   strcat(szToken,buf);  
									iNextIsLabel=0;
						}		
						else  
						  strcpy(szToken,buf);  
			   if(strcmp(szToken,"GOTO")==0 || strcmp(szToken,"goto")==0)
						  iNextIsLabel=1;
						strcat(szLineBuf,szToken);
			   i++;
						buf=GetTokByIndex(i,szLine," ");
			}			
   strcat(szLineBuf,"\n");
			strcpy(szLine,	szLineBuf);
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
int isCommonStatement(char *szLine)
{
			if(strstr(SupressNewLine(GetTokByIndex(0,szLine," ")),"COMMON/")!=NULL)
			{
						fprintf(stdout,"\n%s:%d - Sintaxe de declaraçao ""COMMON"" nao aceita pelo prof.\n",szFileProFor,iLineAtual);
			   fprintf(stdout,"Conteudo da Linha [%s]\n\n",SupressNewLine(szLine));
						exit(1);
   } 

   if (strcmp(SupressNewLine(GetTokByIndex(0,szLine," ")),"COMMON")==0)
			  return(1);
			return(0);		
}
///////////////////////////////////////////////////////////////////////////////
int IsAlreadyAppendedCommonFortranVar(char *szNomeVar,char *szNomeCommon)
{
   int iSearch;
			char szBuffTemp[50];
			strcpy(szBuffTemp,szNomeVar);
   iSearch=0;
   while(iSearch<iEndCommonVariableList)
			{
			   if(strcmp(stListaCommonVariables[iSearch].name,szNomeVar)==0 && strcmp(stListaCommonVariables[iSearch].commonname,szNomeCommon)==0)
						{
						  return(1);
						}		
      iSearch++;
			}
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
void AppendInCommonFortranVariableList(char *szNomeVar,char *szContext)
{
   if(iEndCommonVariableList==DEF_MAX_VAR_WORK)
			{
			   fprintf(stdout,"[AppendInCommonFortranVariableList] - Maximo de variaveis Fortran alcançado. Abortando o pre-compilador\n");
						exit(1);
			}
			if(!IsAlreadyAppendedCommonFortranVar(strlower(szNomeVar),szGlobalCommonName))
			{
   			strcpy(stListaCommonVariables[iEndCommonVariableList].name,strlower(szNomeVar));
						strcpy(stListaCommonVariables[iEndCommonVariableList].commonname,szGlobalCommonName);
						// fprintf(stdout,"              Variavel FORTRAN COMMON -> %s - %s\n",szNomeVar,szGlobalCommonName);
   			iEndCommonVariableList++;
			}
}
///////////////////////////////////////////////////////////////////////////////
void SaveCommonVariables(char *szLine)
{
   int i;
			char *buf,*buf1;
   if(!isFortranContinuation(szLine) && isCommonStatement(szLine))
			{
			   i=2;
						strcpy(szGlobalCommonName,GetDeclarationItemTokenByIndex(1,szLine));
						if(szGlobalCommonName[strlen(szGlobalCommonName)-1]!='/')
						{
									fprintf(stdout,"\n%s:%d - Sintaxe de declaraçao ""COMMON"" nao aceita pelo prof.\n",szFileProFor,iLineAtual);
			      fprintf(stdout,"Conteudo da Linha [%s]\n\n",SupressNewLine(szLine));
									exit(1);
      } 
			}			
			else
			   i=1;
			buf=GetDeclarationItemTokenByIndex(i,szLine);
   while(buf!=NULL)	     					
			{
			   buf1=buf;
						buf1=strstr(buf,",");  if(buf1!=NULL)  buf1[0]=0x00;	
						buf1=strstr(buf,"[");  if(buf1!=NULL)  buf1[0]=0x00;
						buf1=strstr(buf,"(");  if(buf1!=NULL)  buf1[0]=0x00;
						buf1=strstr(buf,";");  if(buf1!=NULL)  buf1[0]=0x00;
						buf1=strstr(buf,"\n"); if(buf1!=NULL)  buf1[0]=0x00;
						buf1=strstr(buf,")");
						if(buf1==NULL)
						  AppendInCommonFortranVariableList(buf,szSubRoutineContext);
						i++;
						buf=GetDeclarationItemTokenByIndex(i,szLine);
			}		
}
///////////////////////////////////////////////////////////////////////////////
int IsCommonFortranVar(char *szNomeVar)
{
   int iSearch;
			char szBuffTemp[50];
			char *buf,*buf1;
			strcpy(szBuffTemp,szNomeVar);

   buf=&szBuffTemp[0];   
			buf1=buf;
			buf1=strstr(buf,",");  if(buf1!=NULL)  buf1[0]=0x00;	
			buf1=strstr(buf,"[");  if(buf1!=NULL)  buf1[0]=0x00;
			buf1=strstr(buf,"(");  if(buf1!=NULL)  buf1[0]=0x00;
			buf1=strstr(buf,";");  if(buf1!=NULL)  buf1[0]=0x00;
			buf1=strstr(buf,"\n"); if(buf1!=NULL)  buf1[0]=0x00;
			buf1=strstr(buf,")");  if(buf1!=NULL)  buf1[0]=0x00;
			strlower(buf);
			iSearch=0;
   while(iSearch<iEndCommonVariableList)
			{
			   if(strcmp(stListaCommonVariables[iSearch].name,szBuffTemp)==0 )
						{
						  return(1);
						}		
      iSearch++;
			}
			return(0);
}
///////////////////////////////////////////////////////////////////////////////
void ProcessProFortranLine(char *szLine,FILE *fhFortran)
{
   static int isSqlDeclareSection; 
   static int isSqlFormerLine; 
			static int isCommonDeclarationFormerLine; 
						
   szLineBuf[0]=0x00;

   if(isCommonDeclarationFormerLine && !isFortranContinuation(szLine) && !isCommonStatement(szLine))
     isCommonDeclarationFormerLine=0;

   if(isExecSqlBeginDeclareSectionStatement(szLine))
			{
			   IdSqlDeclareSection++;
      isSqlDeclareSection=1;
		 			// PutDeclareSectionId(szLine,IdSqlDeclareSection);
		 			// CorrectStartColumnOfExecSql(szLine);
						ChangeLine2PreprocessComment(szLine);
      // fprintf(fhFortran,szLine);
						fputs(szLine,fhFortran);
						return;
			}		

   if(isExecSqlEndDeclareSectionStatement(szLine))
			{
      isSqlDeclareSection=0;
 					// PutDeclareSectionId(szLine,IdSqlDeclareSection);
 					// CorrectStartColumnOfExecSql(szLine);
						ChangeLine2PreprocessComment(szLine);						
      // fprintf(fhFortran,szLine);
						fputs(szLine,fhFortran);
						return;
			}		

   if(isSqlDeclareSection)
			{
	 				// VariablePutDeclareSectionId(szLine,IdSqlDeclareSection);
			   // fprintf(fhFortran,szLine);
						fputs(szLine,fhFortran);
						if(!isFortranComment(szLine) && !isOnlyNewLine(szLine))
						{
						   AdjustFortranMultiVariableDeclaration(szLine,fhFortran);
									// ChangeLine2PreprocessComment(szLine);
      			// fprintf(fhFortran,szLine);
						}
						return;
			}
			
   if(isCommonStatement(szLine))
			{
      isCommonDeclarationFormerLine=1;
						SaveCommonVariables(szLine);
						fputs(szLine,fhFortran);
						return;
			}		

   if(isCommonDeclarationFormerLine && isFortranContinuation(szLine))
			{
      isCommonDeclarationFormerLine=1;
						SaveCommonVariables(szLine);
						fputs(szLine,fhFortran);
						return;
			}		

			
			if(isSqlFormerLine && isFortranContinuation(szLine) && (!isExecSqlStatementAndLabel(szLine)))
			{
			   // if(isExecSqlStatement(szLine))
			      // CorrectStartColumnOfExecSql(szLine);
			   CorrectSqlFortranDoCall(szLine);
						CorrectSqlFortranGoto(szLine);
						ChangeLine2PreprocessComment(szLine);
      // fprintf(fhFortran,szLine);
						fputs(szLine,fhFortran);
						isSqlFormerLine=1;
      return;  
			}
			
   if(isExecSqlStatement(szLine))
   {
     // CorrectStartColumnOfExecSql(szLine);			
			   CorrectSqlFortranDoCall(szLine);
						CorrectSqlFortranGoto(szLine);
						ChangeLine2PreprocessComment(szLine);
      // fprintf(fhFortran,szLine);
						fputs(szLine,fhFortran);
						isSqlFormerLine=1;
      return;  
   }  
			
   if(isExecSqlStatementAndLabel(szLine))
   {
			   CorrectSqlFortranDoCall(szLine);
						CorrectSqlFortranGoto(szLine);
			   strcpy(szLineBuf1,szLine);
      sprintf(szLineBuf,"%s",strtok(szLineBuf1, " "));
      while(strlen(szLineBuf)<6)
        strcat(szLineBuf," ");
      strcat(szLineBuf,"continue\n");
      // fprintf(fhFortran,szLineBuf);
						fputs(szLineBuf,fhFortran);
      strcpy(szLineBuf,"C__PREPROC__      ");
						if(strstr(szLine,"exec")!=NULL)
						  strcat(szLineBuf,strstr(szLine,"exec"));
						if(strstr(szLine,"EXEC")!=NULL)
						  strcat(szLineBuf,strstr(szLine,"EXEC"));
      // fprintf(fhFortran,szLineBuf);
						fputs(szLineBuf,fhFortran);
						isSqlFormerLine=1;
      return;
   }  
			
			isSqlFormerLine=0;  
   // fprintf(fhFortran,szLine);
			fputs(szLine,fhFortran);
}
///////////////////////////////////////////////////////////////////////////////
char *SupressNonValidCharacters(char *szLine)
{
   int i,fim;
			fim = strlen(szLine);
			i=0;
			while(i<fim)
			{
			   if(szLine[i]=='\t')
						  szLine[i]=' ';
			   i++;
			}
			return(szLine);
}
///////////////////////////////////////////////////////////////////////////////
void PrepareProFortranFile(void)
{
  FILE *fhProFor,*fhFortran;

  fprintf(stdout,"Convertendo arquivo [%s] em um formato adequado ao conversor FORTRAN->C\n",szFileProFor);
  fprintf(stdout,"-----------------------------------------------------------------------\n");
  
		strcpy(szFileFortran,IgnoreExtension(szFileProFor));
  strcat(szFileFortran,".f");
  
  fhProFor  = OpenFile(szFileProFor,"rb",1);
  fhFortran  = OpenFile(szFileFortran,"wb",1);
		iLineAtual=0;
		strcpy(szAtualFile,szFileProFor);
  fgets(szLine, MAX_LINE, fhProFor);
		MsgDebugLine(szLine);
  while(!feof(fhProFor))
  {
					SupressNonValidCharacters(szLine);
     ProcessProFortranLine(szLine,fhFortran);   
     fgets(szLine, MAX_LINE, fhProFor);
					MsgDebugLine(szLine);
  }
  fclose(fhProFor); 
  fclose(fhFortran); 
  fprintf(stdout,"-----------------------------------------------------------------------\n");
  fprintf(stdout,"Arquivo [%s] pronto para ser convertido em C\n",szFileFortran);
}
///////////////////////////////////////////////////////////////////////////////
void TestaExtensao(char *szfile)
{
    if(strstr(szfile,".pfo")==NULL)
				{
				   fprintf(stdout,"o arquivo deve ter extensao .pfo\n"); 
							exit(1);
    }
}
///////////////////////////////////////////////////////////////////////////////
void CheckArgs(int argc,char **argv)
{
		iDebugMode=0;
		iSqlLogMode=0;
  if(argc<2 && argc>4)
  {
    fprintf(stdout,"Preprocessador FORTRAN -> C\n");
    fprintf(stdout,"Autor: Andre Rezende R:9361\n\n");
    fprintf(stdout,"Uso:\n\n");
    fprintf(stdout,"prof entrada.pfo\n\n");
    fprintf(stdout,"Modo Verbose:\n");
    fprintf(stdout,"prof -v entrada.pfo\n\n");
    fprintf(stdout,"Gerar executavel com arquivo de log sql:\n");
    fprintf(stdout,"prof -log /diretorio/arq.log\n\n");
    exit(1);
  } 
		if(argc==3 && strcmp(argv[1],"-v")!=0)
		{
    fprintf(stdout,"Preprocessador FORTRAN -> C\n");
    fprintf(stdout,"Autor: Andre Rezende R:9361\n\n");
    fprintf(stdout,"Uso:\n\n");
    fprintf(stdout,"prof entrada.pfo\n\n");
    fprintf(stdout,"Modo Verbose:\n");
    fprintf(stdout,"prof -v entrada.pfo\n\n");
    fprintf(stdout,"Gerar executavel com arquivo de log sql:\n");
    fprintf(stdout,"prof -log /diretorio/arq.log entrada.pfo\n\n");
    exit(1);
		}
		if(argc==4 && strcmp(argv[1],"-log")!=0)
		{
    fprintf(stdout,"Preprocessador FORTRAN -> C\n");
    fprintf(stdout,"Autor: Andre Rezende R:9361\n\n");
    fprintf(stdout,"Uso:\n\n");
    fprintf(stdout,"prof entrada.pfo\n\n");
    fprintf(stdout,"Modo Verbose:\n");
    fprintf(stdout,"prof -v entrada.pfo\n\n");
    fprintf(stdout,"Gerar executavel com arquivo de log sql:\n");
    fprintf(stdout,"prof -log /diretorio/arq.log entrada.pfo\n\n");
    exit(1);
		}
		if(argc==4)
		{
					iDebugMode=0;
					iSqlLogMode=1;
					TestaExtensao(argv[3]);
     strcpy(szFileProFor,argv[3]);
	    strcpy(sz_proforSqlTraceOutput,argv[2]);
		}		
		if(argc==3)
		{
		   iDebugMode=1;
					TestaExtensao(argv[2]);
     strcpy(szFileProFor,argv[2]);
		}		
		if(argc==2)
		{
		   iDebugMode=0;
					TestaExtensao(argv[1]);
     strcpy(szFileProFor,argv[1]);
		}		

  fprintf(stdout,"Preprocessador FORTRAN -> C\n");
  fprintf(stdout,"Autor: Andre Rezende R:9361\n\n");
				
}
///////////////////////////////////////////////////////////////////////////////
FILE *OpenFile(const char *path, const char *mode,int priority)
{
   FILE *tmp = fopen(path,mode);
   if(tmp==0 && priority==1)
   {
     fprintf(stdout,"Erro abrindo arquivo %s\n",path);
     exit(1);
   }  
   if(tmp==0 && priority==0)
     fprintf(stdout,"Erro abrindo arquivo %s\n",path);
}
///////////////////////////////////////////////////////////////////////////////
/*
 * Function: strupper
 * Purpose: Converts a string to upper case--replaces strupr().
 * Parameters: string--The string to convert.
 * Return value: The modified string--actually the same as the parameter as the string is manipulated directly.
 */
char *strupper(char *string)
{
	unsigned int i;
	for (i=0; i<strlen(string); i++) if (isalpha(string[i]) && islower(string[i])) string[i]=toupper(string[i]);
	return string;
}
///////////////////////////////////////////////////////////////////////////////
/*
 * Function: strlower
 * Purpose: Converts a string to lower case--replaces strlwr().
 * Parameters: string--The string to convert.
 * Return value: The modified string--actually the same as the parameter as the string is manipulated directly.
 */
char *strlower(char *string)
{
	unsigned int i;
	for (i=0; i<strlen(string); i++) if (isalpha(string[i]) && isupper(string[i])) string[i]=tolower(string[i]);
	return string;
}
///////////////////////////////////////////////////////////////////////////////
/*
 * Function: strcicmp
 * Purpose: Compares two strings case-insensitively---replaces stricmp() under DOS and strcasecmp() under Unix.
 * Parameters: string1, string2--The strings to compare.
 * Return value: negative if string1<string2, zero if string1==string2 and positive if string1>string2.
 */
int strcicmp(char *string1, char *string2)
{
	strlower(string1);
	strlower(string2);
	return strcmp(string1, string2);
}
///////////////////////////////////////////////////////////////////////////////
