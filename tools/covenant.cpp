#include <cstdio>
#include <cstdlib>

#include <boost/program_options.hpp>

#include <Common.h>
#include <parser/ParseUtils.h>
#include <parser/ParseProblem.h>
#include <CFG.h>
#include <Solver.h>
#include <regsolver/Product.h>

using namespace std;
using namespace covenant;

namespace po = boost::program_options;

int main (int argc, char** argv){

  typedef Sym edge_sym_t;
  typedef Solver<edge_sym_t> solver_t;
  typedef RegSolver<edge_sym_t> reg_solver_t;

  // Main flags
  int max_cegar_iter;
  GeneralizationMethod gen;
  AbstractMethod abs;
  // Heuristics flags
  // - regular solver will try to find a witness starting from this length
  int shortest_witness;
  // - increase the witness length after n iterations -1 disable this option.
  int freq_incr_witness;
  // - increment the witness length by n but only if freq_incr_witness >= 0
  unsigned incr_witness;
  // Other flags
  bool is_dot_enabled=false;

  string header("Covenant: semi-decider for intersection of context-free languages\n");
  header += string ("Authors : G.Gange, J.A.Navas, P.Schachte, H.Sondergaard, and P.J.Stuckey\n");
  header += string ("Usage   : covenant [Options] file");

  po::options_description config("Options");

  config.add_options()
      ("input-file,f",  po::value<string>(), "input file")
      ("help,h", "print help message")
      ("dot,d" , 
       "print abstractions, witnesses, refinements and disjointness proof in dot format")
      ("verbose,v" , "verbose mode")
      ("iter,i",  po::value<int>(&max_cegar_iter)->default_value(-1), 
       "maximum number of CEGAR iterations (default no limit)")
      ("abs,a",  po::value<AbstractMethod>(&abs)->default_value(CYCLE_BREAKING), 
       "choose abstraction method [sigma-star|cycle-breaking]")
      ("gen,g",  po::value<GeneralizationMethod>(&gen)->default_value(GREEDY), 
       "choose generalization method [greedy|max-gen]")
      ("l",  po::value<int>(&shortest_witness)->default_value(1), 
       "shortest length of the witness (default 0)")
      ("freq-incr-witness",  po::value<int>(&freq_incr_witness)->default_value(-1), 
       "how often increasing witnesses' length")
      ("delta-incr-witness",  po::value<unsigned>(&incr_witness)->default_value(1), 
       "how much witnesses' length is incremented")
      ;
 

  po::options_description log("Logging Options");
  log.add_options()
      ("log",  po::value<std::vector<string> >(), "Enable specified log level")
      ;

  po::options_description cmmdline_options;
  cmmdline_options.add(config).add(log);

  po::positional_options_description p;
  p.add("input-file", -1);
  
  po::variables_map vm;

  try {
    po::store(po::command_line_parser(argc, argv).
              options(cmmdline_options).
              positional(p).
              run(), vm);
    po::notify(vm);    
  }
  catch(error &e)
  {
    cerr << "covenant error:" << e << endl;
    return 0;
  }

  if (vm.count("help"))
  {
    std::cout << header << endl << config << endl;
    return 0;
  }  

  std::string in;
  bool file_opened = false;
  if (vm.count ("input-file"))
  {
    std::string infile = vm ["input-file"].as<std::string> ();
    std::ifstream fd;
    fd.open (infile.c_str ());
    if (fd.good ())
    {
      file_opened = true;
      while (!fd.eof ())
      {
        std::string line;
        getline(fd, line);
        in += line;
        in += "\n";
      }
      fd.close ();
    }
  }

  if (!file_opened)
  {
    cout << "Input file not found " << endl;
    return 0;
  }

  if (vm.count("verbose"))
  {
    avy::AvyEnableLog ("verbose");
  }

  if (vm.count("dot"))
  {
    is_dot_enabled = true;
  }
      
  // enable loggers
  if (vm.count("log"))
  {
    vector<string> loggers = vm ["log"].as<vector<string> > ();
    for(unsigned int i=0; i<loggers.size (); i++)
    {
      avy::AvyEnableLog (loggers [i]);
    }
  }

  solver_t::Options opts(max_cegar_iter, 
                         gen, 
                         abs, 
                         is_dot_enabled, 
                         shortest_witness, 
                         freq_incr_witness, 
                         incr_witness);

  CFGProblem problem;
  //StreamParse input(cin);
  StrParse input(in);
  try
  {
    parse_problem(problem, input);
  }
  catch(error &e)
  {
    cerr << "covenant error:" << e << endl;
    return 0;
  }

  if (problem.size() == 0)
  {
    cerr << "covenant error: no CFGs found." << endl;  
    return 0;
  }

  try{

    for(int ii = 0; ii < problem.size(); ii++)
    {
      CFGProblem::Constraint cn(problem[ii]);
      if (cn.vars.empty())
      {
        throw error("expected at least one CFG");
      }
      cn.lang.check_well_formed();
      
      // #ifdef PROBLEM_DEBUG
      //       cout << problem.var_name(cn.vars[0]);
      //       for(unsigned int vi = 1; vi < cn.vars.size(); vi++)
      //         cout<< "." << problem.var_name(cn.vars[vi]);
      //       cout << " \\in " << endl;
      //       cout << cn.lang << endl;
      // #endif
    }
  }
  catch(error &e)
  {
    cerr << "covenant error:" << e << endl;
    return 0;
  }

  // #ifdef PROBLEM_DEBUG
  //   cout << "===============================================\n";
  // #endif
    
  reg_solver_t *solver = new Product<edge_sym_t>();
  try
  {
    solver_t s(problem, solver, opts);
    STATUS res = s.solve();

    switch (res)
    {
      case SAT:
        cout << "======\nSAT\n======\n";
        break;
      case UNSAT:
        cout << "======\nUNSAT\n======\n";
        break;
      default:
        cout << "======\nUNKNOWN\n======\n";  
        break;
    }
      
  }
  catch(error &e)
  {
    cerr << "covenant error:" << e << endl;
  }
  delete solver;

  return 0;
}

