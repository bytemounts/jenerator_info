using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace generator_web.Models
{
    public class ControlAction
    {
        [Key]
        public int Id { get; set; }

        public string userName { get; set; }

        [Required]
        [StringLength(50)]
        public string ActionType { get; set; } = string.Empty; // START, STOP, AUTO, MANUAL, TEST, EMERGENCY_STOP

        [StringLength(200)]
        public string Description { get; set; } = string.Empty;

        [Required]
        public DateTime Timestamp { get; set; } = DateTime.Now;

        [StringLength(45)]
        public string IpAddress { get; set; } = string.Empty; // User's IP address

        public bool IsExecuted { get; set; } = false; // If the command has been executed

        [StringLength(500)]
        public string Result { get; set; } = string.Empty; // Result or response from the generator

        public DateTime? ExecutedAt { get; set; } // when the action was executed

        [StringLength(20)]
        public string Status { get; set; } = "PENDING"; // PENDING, SUCCESS, FAILED
    }
}