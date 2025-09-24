using Microsoft.AspNetCore.Mvc;

namespace generator_web.Controllers
{
    public class GeneratorController : Controller
    {
        public IActionResult Index()
        {
            return View();
        }
    }
}
