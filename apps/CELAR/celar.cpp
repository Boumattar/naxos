
//  Naxos Solver:  A Constraint Programming Library         //
//  Copyright � 2007-2013  Nikolaos Pothitos                //
//  See ../license/LICENSE for the license of the library.  //

#include <naxos.h>
#include <amorgos.h>
#include "heuristics.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>

using namespace std;
using namespace naxos;

int  main (int argc, char *argv[])
{
        try {
                time_t  timeBegin=time(0);
                NsProblemManager  pm;
                pm.realTimeLimit(900);
                if ( argc != 2  &&  argc != 3 ) {
                        cerr << argv[0] << ": correct syntax is: "
                             << argv[0] << " scen_directory [conf]\n";
                        return  1;
                }
                CelarInfo  info;
                double  conf=-1;
                if ( argc  ==  3 ) {
                        istringstream  conf_argument(argv[2]);
                        if ( ! ( conf_argument >> conf ) ) {
                                cerr << argv[0] << ": Wrong conf number `"
                                     << argv[2] << "'!\n";
                                return  1;
                        }
                }
                ifstream  file;
                file.open( ( string(argv[1]) + "/cst.txt" ).c_str() );
                if ( ! file ) {
                        cerr << argv[0] << ": could not open `"
                             << argv[1] << "/cst.txt'!\n";
                        return  1;
                }
                string  str;
                while ( file >> str  &&  str != "a1" )
                        /* VOID */ ;
                if ( str  !=  "a1" ) {
                        cerr << argv[1] << "/cst.txt: Missing `a1'!\n";
                        return  1;
                }
                NsIndex  i, j;
                NsDeque< NsInt >  a(8);
                for (i=0;  i < a.size(); ++i) {
                        if ( ( i != 0 && ! ( file >> str ) )  ||
                             ! ( file >> str)  ||  str != "="  ||
                             ! ( file >> a[i] ) ) {
                                cerr << argv[1] << "/cst.txt: Missing a_i/b_i!\n";
                                return  1;
                        }
                }
                NsDeque< NsInt >  b(4);
                for (i=0;  i < b.size(); ++i)
                        b[i]  =  a[i+4];
                a.resize(4);
                file.close();
                file.open( ( string(argv[1]) + "/dom.txt" ).c_str() );
                if ( ! file ) {
                        cerr << argv[0] << ": could not open `"
                             << argv[1] << "/dom.txt'!\n";
                        return  1;
                }
                NsIndex  index;
                NsIndex  cardinality;
                NsDeque< NsDeque<NsInt> >  domains;
                while  ( file >> index ) {
                        if ( ! ( file >> cardinality ) ) {
                                cerr << argv[1] << "/dom.txt: " << index
                                     << ": Missing cardinality!\n";
                                return  1;
                        }
                        domains.push_back( NsDeque<NsInt>(cardinality) );
                        for (i=0;  i < cardinality;  ++i) {
                                if ( ! ( file >> domains[index][i] ) ) {
                                        cerr << argv[1] << "/dom.txt: " << index
                                             << ": Missing domain value!\n";
                                        return  1;
                                }
                        }
                        //j = 0;
                        //for (i=0;  i < cardinality;  ++i)
                        //	for ( ;  j <= domains[index][i];  ++j)
                        //		domainsNext[index][j]  =  domains[index][i];
                        //j = domains[index][cardinality-1];
                        //for (i=cardinality-1;  i >= 0;  --i)
                        //	for ( ;  j >= domains[index][i];  --j)
                        //		domainsPrevious[index][j]  =  domains[index][i];
                }
                file.close();
                NsDeque< NsDeque<NsInt> >  domainsPrevious(domains.size()),
                         domainsNext(domains.size());
                NsIntVarArray  AllVars, vObjectiveTerms;
                NsDeque<NsIndex>  indexToVar;
                NsIndex  varIndex, varIndexY, varDomain, cost;
                NsInt  varInitial, difference;
                file.open( ( string(argv[1]) + "/var.txt" ).c_str() );
                if ( ! file ) {
                        cerr << argv[0] << ": could not open `"
                             << argv[1] << "/var.txt'!\n";
                        return  1;
                }
                while ( getline(file,str) ) {
                        istringstream  line(str);
                        if ( ! ( line >> varIndex >> varDomain ) ) {
                                cerr << argv[1] << "/var.txt: Syntax error!\n";
                                return  1;
                        }
                        for (i=indexToVar.size();  i < varIndex;  ++i)
                                indexToVar.push_back(indexToVar.max_size());
                        indexToVar.push_back( AllVars.size() );
                        if ( line >> varInitial >> cost ) {
                                if ( cost  ==  0 ) {
                                        if ( find(domains[varDomain].begin(),
                                                  domains[varDomain].end(),
                                                  varInitial) ==
                                             domains[varDomain].end() ) {
                                                cerr << argv[1]
                                                     << "/var.txt: Initial value not in domain!\n";
                                                return  1;
                                        }
                                        //Var.push_back( new NsIntVar(pm,varInitial,varInitial) );
                                        AllVars.push_back( NsIntVar(pm,varInitial,varInitial) );
                                } else {
                                        --cost;
                                        //Var.push_back( new NsIntVar(pm,
                                        //domains[varDomain][0],
                                        //domains[varDomain][ domains[varDomain].size()-1 ]) );
                                        AllVars.push_back(
                                                NsInDomain(pm, domains[varDomain],
                                                           domainsPrevious[varDomain],
                                                           domainsNext[varDomain]) );
                                        vObjectiveTerms.push_back(
                                                ( AllVars.back() != varInitial ) * b[cost] );
                                }
                        } else {
                                //Var.push_back( new NsIntVar(pm,
                                //	domains[varDomain][0],
                                //	domains[varDomain][ domains[varDomain].size()-1 ]) );
                                AllVars.push_back(
                                        NsInDomain(pm, domains[varDomain],
                                                   domainsPrevious[varDomain],
                                                   domainsNext[varDomain]) );
                        }
                }
                file.close();
                info.varsConnected.resize( AllVars.size() );
                //for (NsIndex i=0;  i < Var.size();  ++i)
                //	cout << "Var[" << i << "] = " << Var[i] << "\n";
                file.open( ( string(argv[1]) + "/ctr.txt" ).c_str() );
                if ( ! file ) {
                        cerr << argv[0] << ": could not open `"
                             << argv[1] << "/ctr.txt'!\n";
                        return  1;
                }
                while ( file >> varIndex >> varIndexY >> str >> str >> difference >> cost ) {
                        //cout << "|" << varIndex << " - " << varIndexY << "| " << str << " " << difference << " (" << cost << ")\n";
                        //cout << "|" << Var[varIndex] << " - " << Var[varIndexY] << "| " << str << " " << difference << " (" << cost << ")\n";
                        if ( str != "="  &&  str != ">" ) {
                                cerr << argv[1] << "/ctr.txt: Invalid operand `"
                                     << str << "'!\n";
                                return  1;
                        }
                        i  =  indexToVar[varIndex];
                        j  =  indexToVar[varIndexY];
                        info.varsConnected[i].push_back(AllVars[j]);
                        if ( cost  ==  0 ) {
                                if ( str  ==  "=" ) {
                                        pm.add( NsAbs( AllVars[i] - AllVars[j] )  ==  difference );
                                } else {
                                        pm.add( NsAbs( AllVars[i] - AllVars[j] )  >  difference );
                                }
                        } else {
                                --cost;
                                if ( str  ==  "=" ) {
                                        vObjectiveTerms.push_back(
                                                ( NsAbs( AllVars[i] - AllVars[j] ) !=
                                                  difference ) * a[cost] );
                                } else {
                                        vObjectiveTerms.push_back(
                                                ( NsAbs( AllVars[i] - AllVars[j] ) <=
                                                  difference ) * a[cost] );
                                }
                        }
                }
                file.close();
                NsIntVar  vObjective = NsSum(vObjectiveTerms);
                pm.minimize( vObjective );
                VarHeurCelar  varHeur(info, conf);
                ValHeurCelar  valHeur(AllVars, info, conf);
                pm.addGoal( new AmDfsLabeling(AllVars, &varHeur, &valHeur) );
                NsDeque<NsInt>  bestAllVars(AllVars.size());
                NsInt  bestObjective=-1;
                double  bestTime=-1;
                while ( pm.nextSolution()  !=  false ) {
                        bestTime  =  difftime(time(0),timeBegin);
                        bestObjective  =  vObjective.value();
                        for (i=0;  i < bestAllVars.size();  ++i)
                                bestAllVars[i]  =  AllVars[i].value();
                }
                //cout << "                    \r" << ++solutions << " solutions of cost " << vObjective.value() << flush;
                if ( bestObjective  !=  -1 ) {
                        //cout << argv[1] << "\t" << conf << "\t" << bestObjective << "\t" << bestTime << "\n";
                        cout << bestTime << "\t" << bestObjective << "\t";
                        pm.printCspParameters();
                        ofstream  fileSolution( ( string(argv[1]) + "/sol.txt" ).c_str() );
                        for (i=0;  i < indexToVar.size();  ++i)
                                if ( indexToVar[i]  != indexToVar.max_size() )
                                        fileSolution << i << "\t" << bestAllVars[indexToVar[i]] << "\n";
                        fileSolution.close();
                }
                //for (i=0;  i < Var.size();  ++i)
                //	if ( Var[i]  !=  0 )
                //		delete  Var[i];
        } catch (exception& exc) {
                cerr << exc.what() << "\n";
        } catch (...) {
                cerr << "Unknown exception" << "\n";
        }
}