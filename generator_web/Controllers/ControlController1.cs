using generator_web.Models;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;

namespace generator_web.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class ControlController : ControllerBase
    {
        private readonly AppDbContext _context;

        public ControlController(AppDbContext context)
        {
            _context = context;
        }

        [HttpPost("execute")]
        public async Task<IActionResult> ExecuteAction([FromBody] ControlActionRequest request)
        {
            try
            {
                var userIdClaim = User.FindFirst(ClaimTypes.NameIdentifier);
                if (userIdClaim == null)
                {
                    return Unauthorized(); // pas connecté
                }

                int userId = int.Parse(userIdClaim.Value);

                // Create action and add to database
                var action = new ControlAction
                {
                    ActionType = request.ActionType,
                    Description = $"Action {request.ActionType} executed",
                    Timestamp = DateTime.Now,
                    IsExecuted = true,
                    Status = "SUCCESS",
                    Result = "Action saved"
                };

                _context.ControlActions.Add(action);
                await _context.SaveChangesAsync();

                return Ok(new { success = true, message = $"Action {request.ActionType} Saved" });
            }
            catch (Exception ex)
            {
                return Ok(new { success = false, message = ex.Message });
            }
        }
    }

    public class ControlActionRequest
    {
        public string ActionType { get; set; } = "";
    }
}